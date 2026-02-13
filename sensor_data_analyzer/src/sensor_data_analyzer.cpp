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
state_vector_t state_vector;
D_GNSS_coordinates_t coordinates;
measurement_data_t obs;
float3vector external_induction;

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
  itoa(
      (
	  sizeof( measurement_data_t) +
	  sizeof( float3vector) +
	  sizeof( D_GNSS_coordinates_t) +
	  sizeof( uint32_t) +
	  sizeof( state_vector_t)
      ) / sizeof(uint32_t), size_string, 10);
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

  // initialize empty EEPROM data file
  memset ((uint8_t*) permanent_data_file_storage, 0xff,
	  EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
  success =
      permanent_data_file.set_memory_to_existing_data (
	  (uint32_t*) permanent_data_file_storage,
	  (uint32_t*) permanent_data_file_storage + EEPROM_FILE_SYSTEM_SIZE);

  bool need_to_dump_EEPROM_data = true;

  while ( in_file.read (
      (char*) &next_block_identifier,
      sizeof(next_block_identifier)))
    {
      int size = flexible_log_file_t::verify_record_get_size (
	  next_block_identifier);

      if (size == 0) // error, format not recognized
	break;

      if( (next_block_identifier & 0xffff) == 0xffff)
	{
	  uint32_t extended_header[2]; // 32bit identifier and 32bit length
	  in_file.read ((char*) extended_header, sizeof( extended_header));
	  size = flexible_log_file_t::verify_extended_record_get_size ( next_block_identifier, extended_header[0], extended_header[1]);
	  if( size == 0)
	    break; // error
	  next_block_identifier = extended_header[0]; // replace short ID by extended ID
	}
      else
	next_block_identifier &= 0xff; // keep only the ID

      if( size > MAX_SUPPORTED_RECORD_SIZE_WORDS)
	break;

      in_file.read ((char*) in_data, size * sizeof(uint32_t));
      unsigned bytes_read = in_file.gcount ();
      if (bytes_read != (size * sizeof(uint32_t)))
	break;

      switch ( next_block_identifier)
	{
// ***********************************************************************************************************
	case EEPROM_FILE:
	  memset ((uint8_t*) permanent_data_file_storage, 0xff,
		  EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
	  memcpy (permanent_data_file_storage, in_data, bytes_read);
	  success = permanent_data_file.file_is_consistent();
	  if (not success)
	    {
	      cout << "EEPROM data file not consistent\n";
	      return -1;
	    }

	  cout << "EEPROM data read:\n";
	  permanent_data_file.dump_all_entries();
	  need_to_dump_EEPROM_data = false;

	  break;
// ***********************************************************************************************************
	case EEPROM_FILE_RECORD:
	  {
	    EEPROM_file_system_node *candidate = (EEPROM_file_system_node *)in_data;
	    if( not EEPROM_file_system::node_is_consistent( candidate))
		{
		  cout << "EEPROM data entry not consistent\n";
		  return -1;
		}
	    EEPROM_file_system_node::ID_t id = candidate->id;
	    permanent_data_file.store_data( id, candidate->size - 1, (void *)(candidate+1));
	  }
	  break;
// ***********************************************************************************************************
	case BASIC_SENSOR_DATA:
	  if( need_to_dump_EEPROM_data)
	    {
	      need_to_dump_EEPROM_data = false;
	      cout << "EEPROM data read:\n";
	      permanent_data_file.dump_all_entries();
	    }

	  ++records;

	  assert( size * sizeof(uint32_t) == sizeof( measurement_data_t));
	  memcpy( (uint8_t *)&obs, in_data, size * sizeof(uint32_t));

	  if( measurement_initialized)
	    {
	      organizer->on_new_pressure_data( obs.static_pressure, obs.pitot_pressure);
	      organizer->update_at_100_Hz( obs, system_state, external_induction);
	    }
	  if( ++counter_10Hz == 10)
	    {
	      counter_10Hz = 0;
	      bool landing_detected = organizer->update_at_10Hz ( coordinates, obs);

	      if (landing_detected)
		{
		  organizer->cleanup_after_landing();
		  printf ("landed at log time %d minutes.\n", records / 6000);
		}
	    }

	  if( organizer != 0) // after initialization
	    {
	      organizer->report_data ( state_vector);
	      out_file.write ( (const char*)&obs, sizeof( obs));
	      out_file.write ( (const char*)&external_induction, sizeof( external_induction));
	      out_file.write ( (const char*)&coordinates, sizeof(coordinates));
	      out_file.write ( (const char*)&system_state, sizeof(system_state));
	      out_file.write ( (const char*)&state_vector, sizeof(state_vector_t));
	    }

	  break;
// ***********************************************************************************************************
	case MAGNETOMETER_DATA:
	  assert( size * sizeof(uint32_t) == sizeof( float3vector));
	  memcpy( (uint8_t *)&(external_induction), in_data, size * sizeof(uint32_t));
	  break;
// ***********************************************************************************************************
	  case D_GNSS_DATA:
	    {
	    assert( size * sizeof(uint32_t) == sizeof( D_GNSS_coordinates_t));
	    memcpy( (uint8_t *)&( coordinates), in_data, size * sizeof(uint32_t));

#if PRINT_GNSS_RATE
	      static int old;
	      float delta = state_vector.obs.c.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = state_vector.obs.c.nano;
#endif

	      if( not measurement_initialized)
	      {
		measurement_initialized = true;
		organizer = new organizer_t;
		organizer->initialize_before_measurement ();

		organizer->initialize_after_first_measurement ( coordinates, obs);
	      }
	    organizer->update_GNSS_data ( coordinates);
	    state_vector.satfix = coordinates.sat_fix_type;
	    }
	    break;
// ***********************************************************************************************************
	    case GNSS_DATA:
	      {
	      assert( size * sizeof(uint32_t) == sizeof( GNSS_coordinates_t));

	      memcpy( (uint8_t *)&( coordinates), in_data, size * sizeof(uint32_t));

#if PRINT_GNSS_RATE
	      static int old;

	      float delta = input.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = input.nano;
#endif

	      coordinates.relPosHeading = 0;
	      coordinates.relPosNED = float3vector();

	      if( not measurement_initialized)
		{
		  measurement_initialized = true;
		  organizer = new organizer_t;
		  organizer->initialize_before_measurement ();

		  organizer->initialize_after_first_measurement ( coordinates, obs);
		}
	      organizer->update_GNSS_data ( coordinates);
	      state_vector.satfix = coordinates.sat_fix_type;
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
