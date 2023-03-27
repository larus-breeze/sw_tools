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
#include "old_data_structures.h"
#include "system_state.h"
#include "magnetic_induction_report.h"
#include "ascii_support.h"

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
    realtime_with_TCP_server = open_TCP_port();

  bool USB_active = false;

  if( realtime_with_TCP_server)
    {
      USB_active = open_USB_serial ( (char*)"/dev/ttyUSB0");
      realtime_with_TCP_server = accept_TCP_client(true);
    }

  // cut off file extension
  char basename[100];
  strcpy( basename, argv[1]);
#if NEW_DATA_FORMAT
  char * dot = strchr( basename, '.');
  if( (dot != 0) && (dot[1] == 'f')) // old format: filename.f37.EEPROM new: filename.EEPROM
    *dot=0; // cut off .f37 extension
#endif

  if( read_EEPROM_file ( basename) == EXIT_FAILURE)
    {
      cout << "Unable to open EEPROM file";
      return -1;
    }

  organizer_t organizer;

  streampos size = file.tellg ();
#if NEW_DATA_FORMAT
  observations_type  * in_data;
  in_data = (observations_type*) new char[size];
  unsigned records = size / sizeof(observations_type);
#else
  old_input_data_t  * in_data;
  in_data = (old_input_data_t*) new char[size];
  unsigned records = size / sizeof(old_input_data_t);
#endif

  size_t outfile_size = records * sizeof(output_data_t);
  output_data = (output_data_t*) new char[outfile_size];

  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();

// ************************************************************

  organizer.initialize_before_measurement();

#if NEW_DATA_FORMAT

  int32_t nano = 0;
  unsigned fife_hertz_counter = 0;
  int delta_time;

  output_data[0].m = in_data[0].m;
  output_data[0].c = in_data[0].c;
#else
  new_format_from_old( output_data[0].m, output_data[0].c, in_data[0]);
#endif

  organizer.initialize_after_first_measurement( output_data[0]);
  organizer.update_GNSS_data(output_data[0].c);

  unsigned counter_10Hz = 10;
  auto until = awake_time(std::chrono::steady_clock::now());  // start with now + 100ms

  unsigned count;
  for ( count = 1; count < records; ++count)
    {
#if NEW_DATA_FORMAT
      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;
#else
      new_format_from_old( output_data[count].m, output_data[count].c, in_data[count]);
#endif
      organizer.on_new_pressure_data( output_data[count]);

#if NEW_DATA_FORMAT
      if (output_data[count].c.nano != nano) // 10 Hz by GNSS
	{
	  delta_time = output_data[count].c.nano - nano;
	  if( delta_time < 0 )
	    delta_time += 1000000000;
	  nano = output_data[count].c.nano;

	  organizer.update_GNSS_data(output_data[count].c);
	  counter_10Hz = 1; // synchronize the 10Hz processing as early as new data are observed
	}
#else
      if( count % 10 ==0)
	{
	  organizer.update_GNSS_data(output_data[count].c);
	  counter_10Hz = 1; // synchronize the 10Hz processing as early as new data are observed
	}
#endif

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
		--skiptime;
	      else
		{
		  string_buffer_t buffer;
		  format_NMEA_string( (const output_data_t&) *(output_data+count), buffer);
		  write_TCP_port( buffer.string, buffer.length);

		  if( USB_active)
		      CAN_output( (const output_data_t&) *(output_data+count));

      if (until <= std::chrono::steady_clock::now())
                    until = awake_time(std::chrono::steady_clock::now());
      std::this_thread::sleep_until(until);
      until = awake_time(until);
		}
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
    }

  delete[] in_data;
  delete[] output_data;

  if( realtime_with_TCP_server)
    close_TCP_port();
}

void report_magnetic_calibration_has_changed( magnetic_induction_report_t *p_magnetic_induction_report)
{
  magnetic_induction_report_t magnetic_induction_report = *p_magnetic_induction_report;
  char buffer[50];
    char *next = buffer;
    int32_t writtenBytes = 0;

    printf("\n");

    for( unsigned i=0; i<3; ++i)
      {
        char *next = buffer;
        next = my_ftoa (next, magnetic_induction_report.calibration[i].offset);
        *next++=' ';
        next = my_ftoa (next, magnetic_induction_report.calibration[i].scale);
        *next++=' ';
        next = my_ftoa (next, SQRT( magnetic_induction_report.calibration[i].variance));
        *next++=' ';
        *next++=0;
        printf("%s\t", buffer);
      }


    float3vector induction = magnetic_induction_report.nav_induction;
    for( unsigned i=0; i<3; ++i)
      {
        next = my_ftoa (next, induction[i]);
        *next++=' ';
      }

    next = my_ftoa (next, magnetic_induction_report.nav_induction_std_deviation);
    *next++=0;

    printf("%s ", buffer);

    printf( "Dev=%f Inc=%f",
	    atan2(magnetic_induction_report.nav_induction[EAST],magnetic_induction_report.nav_induction[NORTH])*180.0/M_PI,
	    atan2(magnetic_induction_report.nav_induction[DOWN],magnetic_induction_report.nav_induction[NORTH])*180.0/M_PI);

    printf("\n", buffer);
}
