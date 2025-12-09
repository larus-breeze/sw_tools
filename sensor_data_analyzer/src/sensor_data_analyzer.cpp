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
#include "persistent_data_file.h"

#define FILE_SYSTEM_SIZE 1024

node permanent_file[FILE_SYSTEM_SIZE];
file_system permanent_data_file (permanent_file,
				 permanent_file + FILE_SYSTEM_SIZE);
bool using_permanent_data_file = false;


#include "soft_iron_compensator.h"
soft_iron_compensator_t soft_iron_compensator;
void trigger_soft_iron_compensator_calculation()
{
  soft_iron_compensator.calculate();
}

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

bool read_meta_data_file( char * file_path);
void write_permanent_data_file( char * file_name);

int main (int argc, char *argv[])
{
  unsigned skiptime;

#ifndef _WIN32
  // avoid using FE_UNDERFLOW as it may occur occasionally when filters decay
  //  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
  feenableexcept ( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  if ((argc != 2) && (argc != 3))
    {
      printf ("usage: %s infile.f50 [skiptime for TCP version]\n", argv[0]);
      return -1;
    }

  bool realtime_with_TCP_server = argc == 3;
  if (realtime_with_TCP_server)
    skiptime = 10 * atoi (argv[2]); // at 10Hz output rate

  output_data_t *output_data;

  ifstream file (argv[1], ios::in | ios::binary | ios::ate);
  if (!file.is_open ())
    {
      cout << "Unable to open file";
      return -1;
    }

  if (realtime_with_TCP_server)
    {
      realtime_with_TCP_server = open_TCP_port ();
      realtime_with_TCP_server = accept_TCP_client (true);
    }

#ifndef _WIN32
  if (realtime_with_TCP_server)
    {
      open_USB_serial ((char*) "/dev/ttyUSB0");
#if ENABLE_LINUX_CAN_INTERFACE
      CAN_socket_initialize ();
#endif
    }
#endif

  if (not read_meta_data_file (argv[1]))
    {
      printf ("Unable to open metadata file\n");
      return -1;
    }

  organizer_t organizer;

  streampos size = file.tellg ();
  observations_type *in_data;
  in_data = (observations_type*) new char[size];
  unsigned records = size / sizeof(observations_type);

  size_t outfile_size = records * sizeof(output_data_t);
  output_data = (output_data_t*) new char[outfile_size];

  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();

// ************************************************************

  organizer.initialize_before_measurement ();

  int32_t nano = 0;
  int delta_time;

  output_data[0].m = in_data[0].m;
  output_data[0].c = in_data[0].c;

  organizer.update_GNSS_data (output_data[0].c);
  organizer.update_magnetic_induction_data (output_data[0].c.latitude,
					    output_data[0].c.longitude);

  unsigned counter_10Hz = 10;
  auto until = awake_time (std::chrono::steady_clock::now ()); // start with now + 100ms

  bool have_GNSS_fix = false;

  unsigned count;
  for (count = 1; count < records; ++count)
    {
      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;
      organizer.on_new_pressure_data (output_data[count].m.static_pressure,
				      output_data[count].m.pitot_pressure);

      if (have_GNSS_fix == false)
	{
	  if (output_data[count].c.sat_fix_type > 0)
	    {
	      organizer.update_magnetic_induction_data (
		  output_data[count].c.latitude,
		  output_data[count].c.longitude);
	      organizer.initialize_after_first_measurement (output_data[count]);
	      have_GNSS_fix = true;
	    }
	}

      if (output_data[count].c.nano != nano) // 10 Hz by GNSS
	{
	  delta_time = output_data[count].c.nano - nano;
	  if (delta_time < 0)
	    delta_time += 1000000000;
	  nano = output_data[count].c.nano;

	  organizer.update_GNSS_data (output_data[count].c);
	  counter_10Hz = 1; // synchronize the 10Hz processing as early as new data are observed
	}

      organizer.update_every_10ms (output_data[count]);

      --counter_10Hz;
      if (counter_10Hz == 0)
	{
	  bool landing_detected = organizer.update_every_100ms (
	      output_data[count]);
	  if (landing_detected)
	    printf ("landed at log time %d minutes.\n", count / 6000);
	  counter_10Hz = 10;
	}

      organizer.report_data (output_data[count]);

      if (count % 10 == 0)
	{
	  if (realtime_with_TCP_server)
	    {
	      if (skiptime > 0)
		{
		  --skiptime;
		  continue;
		}

	      string_buffer_t buffer;
	      buffer.length = 0;

	      if (count % 40 == 0)
		format_NMEA_string_fast (
		    (const output_data_t&) *(output_data + count), buffer,
		    true);

	      if (count % 160 == 0)
		format_NMEA_string_slow (
		    (const output_data_t&) *(output_data + count), buffer);

	      if (buffer.length != 0)
		write_TCP_port (buffer.string, buffer.length);

#if ENABLE_LINUX_CAN_INTERFACE
	      CAN_output ((const output_data_t&) *(output_data + count), true);
#endif

	      if (until <= std::chrono::steady_clock::now ())
		until = awake_time (std::chrono::steady_clock::now ());
	      std::this_thread::sleep_until (until);
	      until = awake_time (until);
	    }
	}
    }

  printf ("%d records\n", count);

  // create file name for the data output file
  char buf[200];
  char ascii_len[10];
  sprintf (ascii_len, "%d", (int) (sizeof(output_data_t) / sizeof(float)));
  strcpy (buf, argv[1]);
  strcat (buf, ".f");
  strcat (buf, ascii_len);

  if (!realtime_with_TCP_server)
    {
      ofstream outfile (buf, ios::out | ios::binary | ios::ate);
      if (outfile.is_open ())
	{
	  outfile.write ((const char*) output_data,
			 records * sizeof(output_data_t));
	  outfile.close ();
	}
    }

  if (soft_iron_compensator.available ())
    {
      const void *data = soft_iron_compensator.get_current_parameters ();
      bool result = permanent_data_file.store_data (
	  SOFT_IRON_PARAMETERS,
	  soft_iron_compensator.get_parameters_size () / sizeof(float32_t),
	  data);
      assert(result == true);
    }

  write_permanent_data_file( argv[1]);

  delete[] in_data;
  delete[] output_data;

  if (realtime_with_TCP_server)
    close_TCP_port ();
}

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

  printf ("\n");
}

bool CAN_gateway_poll(CANpacket&, unsigned int)
{
  return false; // presently just an empty stub
}

bool read_meta_data_file (char *file_path)
{
  char path[100];
  strcpy (path, file_path);
  char *slash = strrchr (path, '/');
  if (slash != 0)
    slash[1] = 0;
  else
    {
      printf ("Unable to open *.f37 data file - exiting");
      exit (-1);
    }

  strcat (path, "configuration_data_file.dat");

  // FIRST preference: read the configuration data file
  ifstream config_file (path, ios::in | ios::binary | ios::ate);
  if (config_file.is_open ())
    {
      streampos size = config_file.tellg ();
      if (size != (FILE_SYSTEM_SIZE * sizeof(uint32_t)))
	{
	  printf ("configuration_data_file.dat : wrong file size\n");
	  return -1;
	}

      config_file.seekg (0, ios::beg);
      config_file.read ((char*) permanent_file, size);
      config_file.close ();

      bool result = permanent_data_file.setup (permanent_file, FILE_SYSTEM_SIZE);
      assert(result == true);

      assert( permanent_data_file.is_consistent ());
      printf ("config file read\n");
      using_permanent_data_file = true;

      unsigned size_bytes = soft_iron_compensator.get_parameters_size();
      float *soft_iron_parameters = new float[ size_bytes];
      permanent_data_file.retrieve_data( SOFT_IRON_PARAMETERS, size_bytes / sizeof( float32_t), soft_iron_parameters);
      soft_iron_compensator.set_parameters(soft_iron_parameters);
    }
  else
    {
      // SECOND preference: Read the meta-data in "config.EEPROM"
      slash = strrchr (path, '/');
      if (slash != 0)
	slash[1] = 0;
      else
	{
	  printf ("Unable to open configuration - exiting");
	  exit (-1);
	}
      strcat (path, "config");
      if (read_EEPROM_file (path) != EXIT_SUCCESS)
	{
	  // THIRD preference: Read the meta-data accompanying the *.f32 i.e. "*.EEPROM"
	  strcpy (path, file_path);
	  char *dot = strrchr (path, '.');
	  if (dot)
	    *dot = 0;
	  if (read_EEPROM_file (path) != EXIT_SUCCESS)
	    {
	      printf ("None of the configuration files found, exiting\n");
	      return -1;
	    }
	}

      ensure_EEPROM_parameter_integrity ();

      // migration of the EEPROM values into the configuration data file format
      memset( permanent_file, 0xff, FILE_SYSTEM_SIZE * sizeof( uint32_t));
      bool result = permanent_data_file.setup (permanent_file, FILE_SYSTEM_SIZE);
      assert(result == true);

      float value;

      value = configuration (SENS_TILT_ROLL);
      result = permanent_data_file.store_data (SENS_TILT_ROLL, 1, &value);

      value = configuration (SENS_TILT_PITCH);
      result = permanent_data_file.store_data (SENS_TILT_PITCH, 1, &value);

      value = configuration (SENS_TILT_YAW);
      result = permanent_data_file.store_data (SENS_TILT_YAW, 1, &value);

      value = configuration (PITOT_OFFSET);
      result = permanent_data_file.store_data (PITOT_OFFSET, 1, &value);

      value = configuration (PITOT_SPAN);
      result = permanent_data_file.store_data (PITOT_SPAN, 1, &value);
      assert(result == true);

      value = configuration (QNH_OFFSET);
      result = permanent_data_file.store_data (QNH_OFFSET, 1, &value);
      assert(result == true);

      value = configuration (VARIO_TC);
      result = permanent_data_file.store_data (VARIO_TC, 1, &value);
      assert(result == true);

      value = configuration (VARIO_INT_TC);
      result = permanent_data_file.store_data (VARIO_INT_TC, 1, &value);
      assert(result == true);

      value = configuration (VARIO_P_TC);
      result = permanent_data_file.store_data (VARIO_P_TC, 1, &value);
      assert(result == true);

      value = configuration (WIND_TC);
      result = permanent_data_file.store_data (WIND_TC, 1, &value);
      assert(result == true);

      value = configuration (MEAN_WIND_TC);
      result = permanent_data_file.store_data (MEAN_WIND_TC, 1, &value);
      assert(result == true);

      value = configuration (HORIZON);
      result = permanent_data_file.store_data (HORIZON, round (value));
      assert(result == true);

      value = configuration (MAG_AUTO_CALIB);
      result = permanent_data_file.store_data (MAG_AUTO_CALIB, round (value));
      assert(result == true);

      value = configuration (GNSS_CONFIGURATION);
      result = permanent_data_file.store_data (GNSS_CONFIGURATION,
					       round (value));
      assert(result == true);

      value = configuration (ANT_BASELENGTH);
      result = permanent_data_file.store_data (ANT_BASELENGTH, 1, &value);
      assert(result == true);

      value = configuration (ANT_SLAVE_DOWN);
      result = permanent_data_file.store_data (ANT_SLAVE_DOWN, 1, &value);
      assert(result == true);

      value = configuration (ANT_SLAVE_RIGHT);
      result = permanent_data_file.store_data (ANT_SLAVE_RIGHT, 1, &value);
      assert(result == true);

      {
      float32_t magnetic_calibration_data[] =
	{ configuration (MAG_X_OFF), configuration (MAG_X_SCALE),
	    configuration (MAG_Y_OFF), configuration (MAG_Y_SCALE),
	    configuration (MAG_Z_OFF), configuration (MAG_Z_SCALE),
	    configuration (MAG_STD_DEVIATION) };

      result = permanent_data_file.store_data (
	  MAG_SENSOR_CALIBRATION,
	  sizeof(magnetic_calibration_data) / sizeof(float32_t),
	  magnetic_calibration_data);
      assert(result == true);
      }

      permanent_data_file.dump_all_entries ();
      assert( permanent_data_file.is_consistent());

      char * slash = strrchr( path, '/');
      if( slash != 0)
	slash[1] = 0;
      strcat( path, "configuration_data_file.dat");
      ofstream perm_data_file (path, ios::out | ios::binary | ios::ate);
      if (!perm_data_file.is_open ())
	return false;

      perm_data_file.write ((const char*) permanent_file,
      FILE_SYSTEM_SIZE * sizeof(uint32_t));
      perm_data_file.close ();
      printf ("configuration_data_file.dat written - closing");
      exit (0);
    }
  return true;
}

void write_permanent_data_file( char * file_name)
{
  assert( permanent_data_file.is_consistent() );
  char path[100];
  strcpy( path, file_name);
  char * slash = strrchr( path, '/');
  assert( slash != 0);
  slash[1]=0;
  strcat( path, "configuration_data_file.dat");
  ofstream perm_data_file (path, ios::out | ios::binary | ios::ate);
  if (!perm_data_file.is_open ())
    {
      printf ("cannot open file : configuration_data_file.dat - closing");
      exit (0);
    }

  perm_data_file.write ((const char*) permanent_file,
  FILE_SYSTEM_SIZE * sizeof(uint32_t));
  perm_data_file.close ();
}
