/***********************************************************************//**
 * @file		sensor_data_analyzer.cpp
 * @brief		PC-based Software-In-The-Loop Data Analyzer
 * @author		Dr. Klaus Schaefer
 * @copyright 		Copyright 2021 Dr. Klaus Schaefer. All rights reserved.
 * @license 		This project is released under the GNU Public License GPL-3.0

    <Larus Flight Sensor Firmware>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 **************************************************************************/

#ifdef _WIN32
# include "windows.h"
#else
# include <unistd.h>
#endif
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "math.h"
#include "data_structures.h"
#include "persistent_data.h"
#include "EEPROM_emulation.h"
#include "organizer.h"
#include "NMEA_format.h"
#include <fenv.h>
#include "CAN_output.h"
#include "NMEA_format.h"
#include "TCP_server.h"
#include "USB_serial.h"
#include "system_state.h"
#include "magnetic_induction_report.h"
#include "ascii_support.h"
#include "CAN_socket_driver.h"
#include "CAN_gateway.h"

#if USE_SOFT_IRON_COMPENSATION

#include "soft_iron_compensator.h"
soft_iron_compensator_t soft_iron_compensator;
void trigger_soft_iron_compensator_calculation()
{
  soft_iron_compensator.calculate();
}

#endif

#if 0
#include "compass_calibrator_3D.h"
compass_calibrator_3D_t compass_calibrator_3D;

void trigger_compass_calibrator_3D_calculation(void)
{
  compass_calibrator_3D.calculate();
}
#endif

#ifdef _WIN32
# pragma float_control(except, on)
#endif

using namespace std;

auto awake_time(std::chrono::steady_clock::time_point stime) {
  using std::chrono::operator""ms;
  return stime + 100ms;
}

uint32_t system_state // fake system state here in lack of hardware
  = GNSS_AVAILABLE | MTI_SENSOR_AVAILABE | MS5611_STATIC_AVAILABLE | PITOT_SENSOR_AVAILABLE;

uint32_t UNIQUE_ID[4]={ 0x4711, 0, 0, 0};

void ftoa_test( void);
bool write_soft_iron_parameters( const char * basename);
void read_soft_iron_parameters( const char * basename);

int main (int argc, char *argv[])
{
  unsigned skiptime;

#ifndef _WIN32
  //  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
  // don't enable UNDERFLOW as this can happen regularly when filter outputs decay
  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  if ((argc != 2) && (argc != 3))
    {
      printf ("usage: %s infile.f50 [skiptime for TCP version]\n", argv[0]);
      return -1;
    }

  bool realtime_with_TCP_server = argc == 3;
  if( realtime_with_TCP_server)
    skiptime = 10 * atoi( argv[2]); // at 10Hz output rate

  output_data_t * output_data;

  ifstream file (argv[1], ios::in | ios::binary | ios::ate);
  if (!file.is_open ())
    {
      cout << "Unable to open file";
      return -1;
    }

  if( realtime_with_TCP_server)
    {
      realtime_with_TCP_server = open_TCP_port();
      realtime_with_TCP_server = accept_TCP_client(true);
    }

#ifndef _WIN32
  if( realtime_with_TCP_server)
    {
      open_USB_serial ( (char*)"/dev/ttyUSB0");
#if ENABLE_LINUX_CAN_INTERFACE
      CAN_socket_initialize();
#endif
    }
#endif

  // cut off file extension
  char basename[100];
  strcpy( basename, argv[1]);
  char * dot = strrchr( basename, '.');
  if( (dot != 0) && (dot[1] == 'f')) // old format: filename.f37.EEPROM new: filename.EEPROM
    *dot=0; // cut off .f37 extension

#if LONGTIME_MAG_TEST
// try to read "config.EEPROM" first
  char config_path[200];
  strcpy( config_path, basename);
  char * slash_location = strrchr( config_path, '/');
  *slash_location = 0;
  strcat( config_path, "/config");

  if( read_EEPROM_file ( config_path) == EXIT_FAILURE)
    {
      // try to read the EEPROM file accompanying the data file
      if( read_EEPROM_file ( basename) == EXIT_FAILURE)
	{
	  cout << "Unable to open EEPROM file";
	  return -1;
	}
    }
#else // read the accompanying *.EEPROM only
  if( read_EEPROM_file ( basename) == EXIT_FAILURE)
    {
      cout << "Unable to open EEPROM file";
      return -1;
    }
#endif
  ensure_EEPROM_parameter_integrity();

#if USE_SOFT_IRON_COMPENSATION

  slash_location = strrchr( config_path, '/');
  *slash_location = 0;
  read_soft_iron_parameters( config_path);
#endif

  organizer_t organizer;

  streampos size = file.tellg ();
  observations_type  * in_data;
  in_data = (observations_type*) new char[size];
  unsigned records = size / sizeof(observations_type);

  size_t outfile_size = records * sizeof(output_data_t);
  output_data = (output_data_t*) new char[outfile_size];

  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();

// ************************************************************

  organizer.initialize_before_measurement();

  int32_t nano = 0;
  int delta_time;

  output_data[0].m = in_data[0].m;
  output_data[0].c = in_data[0].c;

  organizer.update_GNSS_data(output_data[0].c);
  organizer.update_magnetic_induction_data( output_data[0].c.latitude, output_data[0].c.longitude);

  unsigned counter_10Hz = 10;
  auto until = awake_time(std::chrono::steady_clock::now());  // start with now + 100ms

  bool have_GNSS_fix = false;

  unsigned count;
  for ( count = 1; count < records; ++count)
    {
      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;
      organizer.on_new_pressure_data( output_data[count]);

      if( have_GNSS_fix == false)
	{
	  if( output_data[count].c.sat_fix_type > 0)
	    {
	      organizer.update_magnetic_induction_data( output_data[count].c.latitude, output_data[count].c.longitude);
	      organizer.initialize_after_first_measurement( output_data[count]);
	      have_GNSS_fix = true;
	    }
	}

      if (output_data[count].c.nano != nano) // 10 Hz by GNSS
	{
	  delta_time = output_data[count].c.nano - nano;
	  if( delta_time < 0 )
	    delta_time += 1000000000;
	  nano = output_data[count].c.nano;

	  organizer.update_GNSS_data(output_data[count].c);
	  counter_10Hz = 1; // synchronize the 10Hz processing as early as new data are observed
	}

      organizer.update_every_10ms( output_data[count]);

      --counter_10Hz;
      if(counter_10Hz == 0)
	{
	  organizer.update_every_100ms( output_data[count]);
	  counter_10Hz = 10;
	}

      organizer.report_data(output_data[count]);

      if( count % 10 == 0)
	{
	  if( realtime_with_TCP_server)
	    {
	      if( skiptime > 0)
		{
		  --skiptime;
		  continue;
		}

	      string_buffer_t buffer;
	      buffer.length=0;

	      if( count % 40 == 0)
		format_NMEA_string_fast( (const output_data_t&) *(output_data+count), buffer, true);

	      if( count % 160 == 0)
		format_NMEA_string_slow( (const output_data_t&) *(output_data+count), buffer);

	      if( buffer.length != 0)
		write_TCP_port( buffer.string, buffer.length);

#if ENABLE_LINUX_CAN_INTERFACE
	      CAN_output( (const output_data_t&) *(output_data+count), true);
#endif

	      if (until <= std::chrono::steady_clock::now())
			    until = awake_time(std::chrono::steady_clock::now());
	      std::this_thread::sleep_until(until);
	      until = awake_time(until);
	    }
	}

    }
  printf ("%d records\n", count);

  char buf[200];
  char ascii_len[10];
  sprintf (ascii_len, "%d", (int)(sizeof(output_data_t) / sizeof(float)));
  strcpy (buf, argv[1]);
  strcat (buf, ".f");
  strcat (buf, ascii_len);

  if( ! realtime_with_TCP_server)
    {
      ofstream outfile (buf, ios::out | ios::binary | ios::ate);
      if (outfile.is_open ())
	{
	  outfile.write ((const char*) output_data,
			 records * sizeof(output_data_t));
	  outfile.close ();
	}

#if LONGTIME_MAG_TEST
      char * path_end = strrchr( buf, '/');
      *path_end=0;
      write_EEPROM_dump(buf); // make new magnetic data permanent
#endif

#if USE_SOFT_IRON_COMPENSATION
      write_soft_iron_parameters( buf);
#endif
    }

  delete[] in_data;
  delete[] output_data;

  if( realtime_with_TCP_server)
    close_TCP_port();
}

#if USE_SOFT_IRON_COMPENSATION

bool write_soft_iron_parameters( const char * basename)
{
  const computation_float_type * data = soft_iron_compensator.get_current_parameters();
  if( data == 0)
    return true;

  char buffer[200];
  strcpy(buffer, basename);
  strcat( buffer, "/soft_iron_parameters.f30");
  ofstream outfile ( buffer, ios::out | ios::binary | ios::ate);
  if ( ! outfile.is_open ())
    return true;

  outfile.write ( (const char *)data, soft_iron_compensator.get_parameters_size());

  outfile.close ();
  return false;
}

void read_soft_iron_parameters( const char * basename)
{
  char buf[200];
  strcpy (buf, basename);
  strcat (buf, "/soft_iron_parameters.f30");

  FILE *fp = fopen(buf, "r");
  if (fp == NULL)
    return;

  char * pdata = new char[soft_iron_compensator.get_parameters_size()];
  if( pdata == 0)
    return;

  unsigned size = fread ( pdata, soft_iron_compensator.get_parameters_size(), 1, fp);
  if( size == 1)
    soft_iron_compensator.set_current_parameters( (const float *)pdata);

  delete [] pdata;
  fclose(fp);
}

#endif

void report_magnetic_calibration_has_changed ( magnetic_induction_report_t *p_magnetic_induction_report, char )
{
  magnetic_induction_report_t magnetic_induction_report = *p_magnetic_induction_report;
  char buffer[50];

  for (unsigned i = 0; i < 3; ++i)
    {
      char *next = buffer;
      next = my_ftoa (next, magnetic_induction_report.calibration[i].offset);
      *next++ = ' ';
      next = my_ftoa (next, magnetic_induction_report.calibration[i].scale);
      *next++ = ' ';
      next = my_ftoa (next,
		      SQRT(magnetic_induction_report.calibration[i].variance));
      *next++ = ' ';
      *next++ = 0;
      printf ("%s\t", buffer);
    }

#if USE_EARTH_INDUCTION_DATA_COLLECTOR

  float3vector induction = magnetic_induction_report.nav_induction;
  for (unsigned i = 0; i < 3; ++i)
    {
      next = my_ftoa (next, induction[i]);
      *next++ = ' ';
    }

  next = my_ftoa (next, magnetic_induction_report.nav_induction_std_deviation);
  *next++ = 0;

  printf( "Dev=%f Inc=%f\n",
      atan2 (magnetic_induction_report.nav_induction[EAST],
	     magnetic_induction_report.nav_induction[NORTH]) * 180.0 / M_PI,
      atan2 (magnetic_induction_report.nav_induction[DOWN],
	     magnetic_induction_report.nav_induction[NORTH]) * 180.0 / M_PI);

#else
  printf ("\n");
#endif
}

bool CAN_gateway_poll(CANpacket&, unsigned int)
{
  return false; // presently just an empty stub
}
