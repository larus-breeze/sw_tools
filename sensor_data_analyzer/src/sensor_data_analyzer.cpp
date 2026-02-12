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
#include "flexible_log_file_implementation.h"
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

int
main (int argc, char *argv[])
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
  char *extension = strrchr (argv[1], '.');
  if (extension == 0)
    {
      cout << "Invalid file extension\n";
      return -1;
    }
  extension += 1;

  if (strcmp (extension, "f37") == 0)
    log_file_format = LEGACY_LOG_FORMAT;
  else if (strcmp (extension, "f32") == 0)
    log_file_format = STD_LOG_FORMAT;
  else if (strcmp (extension, "f35") == 0)
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
  flexible_log_file_implementation_t flex_file (flex_buffer, FLEX_BUF_SIZE);
  char flex_file_path[256];
  strcpy (flex_file_path, argv[1]);
  extension = strrchr (argv[1], '.');
  *extension = 0;
  strcat (flex_file_path, ".fff");
  if (not flex_file.open (flex_file_path))
    {
      printf ("Unable to open output file\n");
      return -1;
    }

  unsigned file_format_version = 0x00000001;
  flex_file.append_record (FILE_FORMAT_VERSION, &file_format_version, 1);
  flex_file.append_record (EEPROM_FILE,
			   (uint32_t*) (permanent_data_file.get_head ()),
			   permanent_data_file.get_size () / sizeof(uint32_t));

  streampos size = file.tellg ();

  unsigned records;

  D_GNSS_coordinates_t c;
  measurement_data_t m;
  state_vector_t s;
  uint32_t system_state = fake_system_state;
  organizer_t organizer;
  organizer.initialize_before_measurement ();
  int32_t nano = 0;
  int delta_time;
  bool have_GNSS_fix = false;
  unsigned counter_10Hz = 10;
  uint32_t old_system_state = system_state;

  bool using_D_GNSS = (configuration (GNSS_CONFIGURATION) == 2)
      || (configuration (GNSS_CONFIGURATION) == 3);

  legacy_observations_type *in_data;
  in_data = (legacy_observations_type*) new char[size];
  records = size / sizeof(legacy_observations_type);
  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();
  for (unsigned i = 0; i < records; ++i)
    {
      m = in_data[i].m;

      c.velocity = in_data[i].c.speed;
      c.relPosNED = in_data[i].c.relPosNED;
      c.relPosHeading = in_data[i].c.relPosHeading;
      c.speed_acc = in_data[i].c.speed_acc;
      c.latitude = in_data[i].c.latitude;
      c.longitude = in_data[i].c.longitude;
      c.GNSS_MSL_altitude = -in_data[i].c.position[DOWN];

      c.year = in_data[i].c.year;
      c.month = in_data[i].c.month;
      c.day = in_data[i].c.day;
      c.hour = in_data[i].c.hour;
      c.minute = in_data[i].c.minute;
      c.second = in_data[i].c.second;

      c.SATS_number = in_data[i].c.SATS_number;
      c.sat_fix_type = in_data[i].c.sat_fix_type;
      c.second = in_data[i].c.second;
      c.nano = in_data[i].c.nano;
      c.geo_sep_dm = in_data[i].c.geo_sep_dm;

      if (c.sat_fix_type > 1)
	system_state |= D_GNSS_AVAILABLE;
      else
	system_state &= ~D_GNSS_AVAILABLE;

      if (old_system_state != system_state)
	{
	  old_system_state = system_state;
	  flex_file.append_record (SENSOR_STATUS, &system_state, 1);
	}

      organizer.on_new_pressure_data (m.static_pressure, m.pitot_pressure);
      flex_file.append_record (BASIC_SENSOR_DATA, (uint32_t*) &m,
			       sizeof(measurement_data_t) / sizeof(uint32_t));

      if (c.nano != nano) // 10 Hz by GNSS
	{
	  delta_time = c.nano - nano;
	  if (delta_time < 0)
	    delta_time += 1000000000;
	  nano = c.nano;

	  organizer.update_GNSS_data (c);

	  if (using_D_GNSS)
	    flex_file.append_record ( D_GNSS_DATA, (uint32_t*) &c, sizeof( D_GNSS_coordinates_t) / sizeof(uint32_t));
	  else
	    flex_file.append_record ( GNSS_DATA, (uint32_t*) &c,   sizeof( GNSS_coordinates_t)   / sizeof(uint32_t));

	  counter_10Hz = 1; // synchronize the 10Hz processing as early as new data are observed
	}

      if (have_GNSS_fix == false)
	{
	  if (c.sat_fix_type > 0)
	    {
	      organizer.update_magnetic_induction_data (c.latitude, c.longitude);
	      organizer.initialize_after_first_measurement (c, m);
	      have_GNSS_fix = true;
	    }
	}
      organizer.update_at_100_Hz (m, system_state, float3vector ());

      if (--counter_10Hz == 0)
	{
	  counter_10Hz = 10;

	  bool landing_detected = organizer.update_at_10Hz (c, m);

	  if (landing_detected)
	    {
	      organizer.cleanup_after_landing ();
	      printf ("landed at log time %d minutes.\n", i / 6000);
	    }
	}
    }

  delete[] in_data;
  flex_file.close ();
  printf ("%d records\n", records);

  exit (0);
}
