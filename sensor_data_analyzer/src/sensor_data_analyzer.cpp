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

#include <unistd.h>
#include <iostream>
#include <fstream>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
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

#define NEW_DATA_FORMAT 1
using namespace std;

int main (int argc, char *argv[])
{
  unsigned skiptime;
  float declination; // todo fixme this variable is somewhat misplaced here

  //  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
  // don't enable UNDERFLOW as this can happen regularly when filter outputs decay
  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

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

  if( read_EEPROM_file (argv[1]) == EXIT_FAILURE)
    {
      cout << "Unable to open EEPROM file";
      return -1;
    }

  organizer_t organizer;

  streampos size = file.tellg ();
#if NEW_DATA_FORMAT
  observations_type  * in_data;
  in_data = (observations_type*) new char[size];
#else
  old_input_data_t  * in_data;
  in_data = (old_input_data_t*) new char[size];
#endif

  unsigned records = size / sizeof(observations_type);
  size_t outfile_size = records * sizeof(output_data_t);
  output_data = (output_data_t*) new char[outfile_size];

  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();

// ************************************************************

  organizer.initialize_before_measurement();

  int32_t nano = 0;
  unsigned fife_hertz_counter = 0;

  int delta_time;

#if NEW_DATA_FORMAT
  output_data[0].m = in_data[0].m;
  output_data[0].c = in_data[0].c;
#else
  *(old_measurement_data_t*)&(output_data[0].m) = in_data[0].m;
  *(old_coordinates_t*)&(output_data[0].c) =      in_data[0].c;
  // patches
  output_data[0].c.sat_fix_type = 3;  // force D-GNSS usage
  output_data[0].c.SATS_number  = 13; // just a joke ...
  output_data[0].c.velocity[DOWN] *= -1.0f;
#endif

  organizer.initialize_after_first_measurement( output_data[0]);
  organizer.update_GNSS_data(output_data[0].c);
  declination = organizer.getDeclination();

  records = 0;

  unsigned counter_10Hz = 10;
  for (unsigned count = 1; count < size / sizeof(observations_type); ++count)
    {
#if NEW_DATA_FORMAT
      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;
#else
      *(old_measurement_data_t*)&(output_data[count].m) = in_data[count].m;
      *(old_coordinates_t*)&(output_data[count].c) =      in_data[count].c;

      // patches
      output_data[count].c.sat_fix_type = 3;  // force D-GNSS usage
      output_data[count].c.SATS_number  = 13; // just a joke ...
      output_data[count].c.velocity[DOWN] *= -1.0f;

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
		  format_NMEA_string( (const output_data_t&) *(output_data+count), buffer, declination);
		  write_TCP_port( buffer.string, buffer.length);

		  if( USB_active)
		      CAN_output( (const output_data_t&) *(output_data+count));

		  timespec want,got;
		  want.tv_nsec = 100000000;
		  want.tv_sec = 0;
		  while( nanosleep( &want, &got))
		    want.tv_nsec = want.tv_nsec - got.tv_nsec;
		}
	    }
	}
      ++records;
    }
  printf ("%d records\n", records);

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
