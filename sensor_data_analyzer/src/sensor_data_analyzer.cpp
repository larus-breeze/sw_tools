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
      printf ("usage: %s infile.f37 (or *.f32 / *.f35)\n", argv[0]);
      return -1;
    }

// ************************************************************

  log_file_format_t log_file_format;
  char * extension = strrchr( argv[1], '.');
  if( extension == 0)
    {
      cout << "Invalid file extension\n";
      return -1;
    }
  extension += 1;

  if( strcmp( extension, "f37") == 0)
    log_file_format = LEGACY_LOG_FORMAT;
  else if( strcmp( extension, "f32") == 0)
    log_file_format = STD_LOG_FORMAT;
  else if( strcmp( extension, "f35") == 0)
    log_file_format = EXTENDED_LOG_FORMAT;
  else
    {
      cout << "Invalid file extension\n";
      return -1;
    }

  ifstream file (argv[1], ios::in | ios::binary | ios::ate);
  if (!file.is_open ())
    {
      cout << "Unable to open file";
      return -1;
    }

  if (not read_meta_data_file (argv[1]))
    {
      printf ("Unable to open meta data file\n");
      return -1;
    }

  uint32_t flex_buffer[FLEX_BUF_SIZE];
  flexible_log_file_t flex_file( flex_buffer, FLEX_BUF_SIZE);
  char flex_file_path[256];
  strcpy( flex_file_path, argv[1]);
  extension = strrchr( argv[1], '.');
  *extension = 0;
  strcat( flex_file_path, ".fff");
  if( not flex_file.open( flex_file_path))
    {
      printf ("Unable to open output file\n");
      return -1;
    }

  unsigned file_format_version = 0x00000001;
  flex_file.append_record( FILE_FORMAT_VERSION, &file_format_version, 1);
  flex_file.append_record( EEPROM_FILE, (uint32_t *)(permanent_data_file.get_head()), permanent_data_file.get_size() / sizeof(uint32_t));

  streampos size = file.tellg ();

  unsigned records;
  size_t outfile_size;
  output_data_t *output_data;

  switch( log_file_format)
  {
    case LEGACY_LOG_FORMAT:
      {
        legacy_observations_type *in_data;
        in_data = ( legacy_observations_type*) new char[size];
        records = size / sizeof( legacy_observations_type);
        outfile_size = records * sizeof(output_data_t);
        output_data = (output_data_t*) new char[outfile_size];
        file.seekg (0, ios::beg);
        file.read ((char*) in_data, size);
        file.close ();
        for( unsigned i = 0; i < records; ++i)
  	{
            output_data[i].obs.m = in_data[i].m;

            output_data[i].obs.c.velocity = in_data[i].c.velocity;
            output_data[i].obs.c.relPosNED = in_data[i].c.relPosNED;
            output_data[i].obs.c.relPosHeading = in_data[i].c.relPosHeading;
            output_data[i].obs.c.speed_acc = in_data[i].c.speed_acc;
            output_data[i].obs.c.latitude = in_data[i].c.latitude;
            output_data[i].obs.c.longitude = in_data[i].c.longitude;
            output_data[i].obs.c.GNSS_MSL_altitude = - in_data[i].c.position[DOWN];

            output_data[i].obs.c.year = in_data[i].c.year;
            output_data[i].obs.c.month = in_data[i].c.month;
            output_data[i].obs.c.day = in_data[i].c.day;
            output_data[i].obs.c.hour = in_data[i].c.hour;
            output_data[i].obs.c.minute = in_data[i].c.minute;
            output_data[i].obs.c.second = in_data[i].c.second;

            output_data[i].obs.c.SATS_number = in_data[i].c.SATS_number;
            output_data[i].obs.c.sat_fix_type = in_data[i].c.sat_fix_type;
            output_data[i].obs.c.second = in_data[i].c.second;
            output_data[i].obs.c.nano = in_data[i].c.nano;
            output_data[i].obs.c.geo_sep_dm = in_data[i].c.geo_sep_dm;

            output_data[i].obs.sensor_status = fake_system_state;
  	    output_data[i].obs.external_magnetometer_reading = {0};
  	    output_data[i].obs.external_magnetometer_reading[1] = 1.0f;
  	}
        delete[] in_data;
        break;
      }
  case EXTENDED_LOG_FORMAT:
    {
      extended_observations_type *in_data;
      in_data = ( extended_observations_type*) new char[size];
      records = size / sizeof( extended_observations_type);
      outfile_size = records * sizeof(output_data_t);
      output_data = (output_data_t*) new char[outfile_size];
      file.seekg (0, ios::beg);
      file.read ((char*) in_data, size);
      file.close ();
      for( unsigned i = 0; i < records; ++i)
	{
	    output_data[i].obs.m = in_data[i].m;
	    output_data[i].obs.c = in_data[i].c;
	    output_data[i].obs.external_magnetometer_reading = in_data[i].external_magnetometer_reading;
	    output_data[i].obs.sensor_status = in_data[i].sensor_status;
	}
      delete[] in_data;
      break;
    }
  case STD_LOG_FORMAT:
    {
      observations_type *in_data;
      in_data = ( observations_type*) new char[size];
      records = size / sizeof( observations_type);
      outfile_size = records * sizeof(output_data_t);
      output_data = (output_data_t*) new char[outfile_size];
      file.seekg (0, ios::beg);
      file.read ((char*) in_data, size);
      file.close ();
      for( unsigned i = 0; i < records; ++i)
	{
	    output_data[i].obs.m = in_data[i].m;
	    output_data[i].obs.c = in_data[i].c;
	    // we did not record external magnetometer data, so this bit needs to be reset
	    output_data[i].obs.sensor_status = in_data[i].sensor_status & ~EXTERNAL_MAGNETOMETER_AVAILABLE;
	    output_data[i].obs.external_magnetometer_reading = {0};
	}
      delete[] in_data;
      break;
    }
  }

  // ************************************************************

  organizer_t organizer;
  organizer.initialize_before_measurement ();

  int32_t nano = 0;
  int delta_time;

  system_state = output_data[0].obs.sensor_status;
  uint32_t old_system_state = system_state;

  flex_file.append_record( SENSOR_STATUS, &system_state, 1);

  organizer.update_GNSS_data (output_data[0].obs.c);
  organizer.initialize_after_first_measurement (output_data[1]);

  unsigned counter_10Hz = 10;

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
    }

  organizer.cleanup_after_landing(); // at least: now !

  flex_file.close();

  printf ("%d records\n", records);

  delete[] output_data;
  exit( 0);
}
