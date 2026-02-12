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

#define MAX_SUPPORTED_RECORD_SIZE_WORDS 256
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
output_data_t output_data;

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

  char buf[512];
  strcpy( buf, argv[1]);
  strcat( buf, ".f");
  {
  char size_string[10];
  itoa( sizeof( output_data_t)/sizeof(uint32_t), size_string, 10);
  strcat( buf, size_string);
  }

  ofstream out_file ( buf, ios::out | ios::binary | ios::ate);
  if( not out_file.is_open ())
    {
      cout << "Unable to open output file\n";
      return -1;
    }

  uint32_t * in_data = new uint32_t[MAX_SUPPORTED_RECORD_SIZE_WORDS];
  unsigned records = 0;
  uint32_t next_block_identifier;

  organizer_t *organizer = 0;
  bool success;
  bool measurement_initialized = false;

  unsigned counter_10Hz=0;

  while ( in_file.read (
      (char*) &next_block_identifier,
      sizeof(next_block_identifier)))
    {
      int size = flexible_log_file_t::verify_record_get_size (
	  next_block_identifier);

      if (size == 0) // error, format not recognized
	break;

      if( size == 255)
	{
	  if( next_block_identifier & 0xff != 255) // wrong format
	    break;

	  uint32_t extended_header[2]; // 32bit identifier and 32bit length
	  in_file.read ((char*) extended_header, sizeof( extended_header));
	  size = extended_header[1] - 3; // subtract node, id, length
	  next_block_identifier = extended_header[0];
	}
      else
	--size; // it included the block identifier

      if( size > MAX_SUPPORTED_RECORD_SIZE_WORDS)
	break;

      in_file.read ((char*) in_data, size * sizeof(uint32_t));
      unsigned bytes_read = in_file.gcount ();
      if (bytes_read != (size * sizeof(uint32_t)))
	break;

      switch ( next_block_identifier & 0xff)
	{
// ***********************************************************************************************************
	case EEPROM_FILE:
	  memset ((uint8_t*) permanent_data_file_storage, 0xff,
		  EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
	  memcpy (permanent_data_file_storage, in_data, bytes_read);
	  success =
	      permanent_data_file.set_memory_to_existing_data (
		  (uint32_t*) permanent_data_file_storage,
		  (uint32_t*) permanent_data_file_storage
		      + EEPROM_FILE_SYSTEM_SIZE);
	  if (not success)
	    {
	      cout << "EEPROM data file not consistent\n";
	      return -1;
	    }

	  cout << "EEPROM data read:\n";
	  permanent_data_file.dump_all_entries();

	  break;
// ***********************************************************************************************************
	case EEPROM_FILE_RECORD:
	  // todo patch implement me !
	  break;
// ***********************************************************************************************************
	case BASIC_SENSOR_DATA:
	  ++records;

	  assert( size * sizeof(uint32_t) == sizeof( output_data.obs.m));
	  memcpy( (uint8_t *)&(output_data.obs.m), in_data, size * sizeof(uint32_t));

	  if( measurement_initialized)
	    {
	      organizer->on_new_pressure_data( output_data.obs.m.static_pressure, output_data.obs.m.pitot_pressure);
	      organizer->update_at_100_Hz(output_data);
	    }
	  if( ++counter_10Hz == 10)
	    {
	      counter_10Hz = 0;
	      bool landing_detected = organizer->update_at_10Hz ( output_data);

	      if (landing_detected)
		{
		  organizer->cleanup_after_landing();
		  printf ("landed at log time %d minutes.\n", records / 6000);
		}
	    }

	  if( organizer != 0) // after initialization
	    {
	      organizer->report_data ( output_data);
	      out_file.write ( (const char*)&output_data, sizeof(output_data_t));
	    }

	  break;
// ***********************************************************************************************************
	case EXTENDED_SENSOR_DATA:
	  ++records;

	  assert( size * sizeof(uint32_t) == sizeof( output_data.obs.m));
	  memcpy( (uint8_t *)&(output_data.obs.m), in_data, size * sizeof(uint32_t));

	  if( measurement_initialized)
	    {
	      organizer->on_new_pressure_data( output_data.obs.m.static_pressure, output_data.obs.m.pitot_pressure);
	      organizer->update_at_100_Hz(output_data);
	    }
	  if( ++counter_10Hz == 10)
	    {
	      counter_10Hz = 0;
	      bool landing_detected = organizer->update_at_10Hz ( output_data);

	      if (landing_detected)
		{
		  organizer->cleanup_after_landing();
		  printf ("landed at log time %d minutes.\n", records / 6000);
		}
	    }

	  if( organizer != 0) // after initialization
	    {
	      organizer->report_data ( output_data);
	      out_file.write ( (const char*)&output_data, sizeof(output_data_t));
	    }

	  break;
// ***********************************************************************************************************
	  case D_GNSS_DATA:
	    {
	    assert( size * sizeof(uint32_t) == sizeof( output_data.obs.c));
	    memcpy( (uint8_t *)&(output_data.obs.c), in_data, size * sizeof(uint32_t));

#if PRINT_GNSS_RATE
	      static int old;
	      float delta = output_data.obs.c.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = output_data.obs.c.nano;
#endif

	      if( not measurement_initialized)
	      {
		measurement_initialized = true;
		organizer = new organizer_t;
		organizer->initialize_before_measurement ();

		organizer->initialize_after_first_measurement (output_data);
	      }
	    organizer->update_GNSS_data ( output_data.obs.c );
	    }
	    break;
// ***********************************************************************************************************
	    case GNSS_DATA:
	      {
	      assert( size * sizeof(uint32_t) == sizeof( GNSS_coordinates_t));

	      GNSS_coordinates_t &input = *(GNSS_coordinates_t *)in_data;

	      output_data.obs.c.sat_fix_type = input.sat_fix_type;
	      output_data.obs.c.SATS_number = input.SATS_number;
	      output_data.obs.c.geo_sep_dm = input.geo_sep_dm;
	      output_data.obs.c.speed_acc = input.speed_acc;
	      output_data.obs.c.pDOP = input.pDOP;

	      output_data.obs.c.year = input.year;
	      output_data.obs.c.month = input.month;
	      output_data.obs.c.day = input.day;
	      output_data.obs.c.hour = input.hour;
	      output_data.obs.c.minute = input.minute;
	      output_data.obs.c.second = input.second;
	      output_data.obs.c.nano = input.nano;

#if PRINT_GNSS_RATE
	      static int old;

	      float delta = input.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = input.nano;
#endif

	      output_data.obs.c.GNSS_MSL_altitude = input.GNSS_MSL_altitude;
	      output_data.obs.c.latitude = input.latitude;
	      output_data.obs.c.longitude = input.longitude;
	      output_data.obs.c.velocity = input.velocity;

	      output_data.obs.c.relPosHeading = 0;
	      output_data.obs.c.relPosNED = float3vector();

	      if( not measurement_initialized)
		{
		  measurement_initialized = true;
		  organizer = new organizer_t;
		  organizer->initialize_before_measurement ();

		  organizer->initialize_after_first_measurement (output_data);
		}
	      organizer->update_GNSS_data ( output_data.obs.c );
	      break;
	      }
// ***********************************************************************************************************
	case SENSOR_STATUS:
	  assert( size == 1);
	  system_state = *in_data;
	  break;
	}
    }

  printf ("%d records read\n", records);
  out_file.close ();
  exit( 0);
}
