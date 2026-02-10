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
#include "persistent_data_file_emulation.h"
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
#include "abstract_EEPROM_storage.h"
#include "flexible_log_file.h"

#include "compass_calibrator_3D.h"
#include "mutex_implementation.h"

#define FLEX_BUF_SIZE 2048

Mutex_Wrapper_Type my_mutex;

magnetic_calculation_data_t temporary_mag_calculation_data;

compass_calibrator_3D_t compass_calibrator_3D( temporary_mag_calculation_data);
compass_calibrator_3D_t external_compass_calibrator_3D( temporary_mag_calculation_data);

void trigger_compass_calibrator_3D_calculation( bool calculate_external_magnetometer)
{
  if( calculate_external_magnetometer)
    external_compass_calibrator_3D.calculate();
  else
    compass_calibrator_3D.calculate();
}

#ifdef _WIN32
# pragma float_control(except, on)
#endif

using namespace std;

auto awake_time(std::chrono::steady_clock::time_point stime)
{
  using std::chrono::operator""ms;
  return stime + 100ms;
}

uint32_t system_state;

uint32_t fake_system_state // fake system state here in lack of hardware
  = GNSS_AVAILABLE | MTI_SENSOR_AVAILABE | MS5611_STATIC_AVAILABLE | PITOT_SENSOR_AVAILABLE;

uint32_t UNIQUE_ID[4]={ 0x4711, 0, 0, 0};


int main (int argc, char *argv[])
{
#ifndef _WIN32
  // avoid using FE_UNDERFLOW as it may occur occasionally when filters decay
  //  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
  feenableexcept ( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  if ((argc != 2))
    {
      printf ("usage: %s infile.larus_log\n", argv[0]);
      return -1;
    }

  ifstream in_file (argv[1], ios::in | ios::binary | ios::ate);
  if (not in_file.is_open ())
    {
      cout << "Unable to open file";
      return -1;
    }

  in_file.seekg (0, ios::beg);

  uint32_t * in_data = new uint32_t[256];
  unsigned records = 0;
  uint32_t next_block_identifier;
  while( in_file.read( (char*)&next_block_identifier, sizeof( next_block_identifier)))
    {
      int size = flexible_log_file_t::verify_record_get_size( next_block_identifier);

      if( size == 0 || size > 255)
	break;

      printf ("%x : %d\n", next_block_identifier & 0xff, size);

      ++records;
      in_file.read ( (char*)in_data, size * sizeof( uint32_t));
      streamsize bytes_read = in_file.gcount();
      if( bytes_read != size * sizeof( uint32_t))
	break;
    }

  printf ("%d records read\n", records);

  // ************************************************************
#if 0
  organizer_t organizer;
  organizer.initialize_before_measurement ();

  int32_t nano = 0;
  int delta_time;

  flex_file.append_record( EEPROM_FILE, (uint32_t *)(permanent_data_file.get_head()), permanent_data_file.get_size());

  system_state = output_data[0].obs.sensor_status;
  uint32_t old_system_state = system_state;
  flex_file.append_record( SENSOR_STATUS, &system_state, 1);

  organizer.update_GNSS_data (output_data[0].obs.c);
  organizer.initialize_after_first_measurement (output_data[1]);

  unsigned counter_10Hz = 10;
  auto until = awake_time (std::chrono::steady_clock::now ()); // start with now + 100ms

  bool have_GNSS_fix = false;

  for ( unsigned count = 1; count < records; ++count)
    {
      system_state = output_data[count].obs.sensor_status;
      if( system_state == 0) // assume no data given
	system_state = fake_system_state;

      if( old_system_state != system_state)
	{
	  old_system_state = system_state;
	  flex_file.append_record( SENSOR_STATUS, &system_state, 1);
	}

      organizer.on_new_pressure_data (output_data[count].obs.m.static_pressure,
				      output_data[count].obs.m.pitot_pressure);

      flex_file.append_record( BASIC_SENSOR_DATA, (uint32_t*)&(output_data[count].obs.m), sizeof( measurement_data_t) / sizeof(uint32_t));

      if (have_GNSS_fix == false)
	{
	  if (output_data[count].obs.c.sat_fix_type > 0)
	    {
	      organizer.update_magnetic_induction_data (
		  output_data[count].obs.c.latitude,
		  output_data[count].obs.c.longitude);
	      organizer.initialize_after_first_measurement (output_data[count]);
	      have_GNSS_fix = true;
	    }
	}

      if (output_data[count].obs.c.nano != nano) // 10 Hz by GNSS
	{
	  delta_time = output_data[count].obs.c.nano - nano;
	  if (delta_time < 0)
	    delta_time += 1000000000;
	  nano = output_data[count].obs.c.nano;

	  organizer.update_GNSS_data (output_data[count].obs.c);

	  flex_file.append_record( GNSS_DATA, (uint32_t*)&(output_data[count].obs.c), sizeof( coordinates_t) / sizeof(uint32_t));

	  counter_10Hz = 1; // synchronize the 10Hz processing as early as new data are observed
	}

      organizer.update_at_100_Hz (output_data[count]);

      --counter_10Hz;
      if (counter_10Hz == 0)
	{
	  counter_10Hz = 10;

	  bool landing_detected = organizer.update_at_10Hz ( output_data[count]);

	  if (landing_detected)
	    {
	      organizer.cleanup_after_landing();
	      printf ("landed at log time %d minutes.\n", count / 6000);
	    }
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

  organizer.cleanup_after_landing(); // at least: now !

  flex_file.close();

  printf ("%d records\n", records);

  if ( realtime_with_TCP_server)
    close_TCP_port ();
  else
    {
      // create file name for the data output file
      char buf[200];
      char ascii_len[10];
      sprintf (ascii_len, "%d", (int) (sizeof(output_data_t) / sizeof(float)));
      strcpy (buf, argv[1]);
      strcat (buf, ".f");
      strcat (buf, ascii_len);

      ofstream outfile (buf, ios::out | ios::binary | ios::ate);
      if (outfile.is_open ())
	{
	  outfile.write ((const char*) output_data, records * sizeof(output_data_t));
	  outfile.close ();
	}
    }

  write_permanent_data_file( argv[1]);

  delete[] output_data;
#endif
  exit( 0);
}
