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
#include "persistent_data.h"
#include "compass_calibrator_3D.h"
#include "mutex_implementation.h"

#define MAX_SUPPORTED_RECORD_SIZE_WORDS 256
#define FLEX_BUF_SIZE 2048

uint32_t buffer[128];
flexible_log_file_implementation_t flex_file( buffer, 128);

bool write_block (uint32_t *p_data, uint32_t size_words)
{
  flex_file.write_block( p_data, size_words);
}

void signal_logger_event( uint32_t event)
{

}

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
measurement_data_t observations;
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

  if ((argc != 2) && (argc != 3))
      {
      printf ("usage: %s infile.larus_log [configuration.lrsx]\n", argv[0]);
      return -1;
    }

  bool have_configuration = false;
  bool write_f37 = false;

  bool start_with_given_configuration;
  if( argc == 3)
    {
      if( strcmp( argv[2], "f37") == 0)
	{
	  write_f37 = true;
	  // initialize empty EEPROM data file
	  memset ((uint8_t*) permanent_data_file_storage, 0xff,
	      EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
	  permanent_data_file.set_memory_to_existing_data (
	      (uint32_t*) permanent_data_file_storage,
	      (uint32_t*) permanent_data_file_storage + EEPROM_FILE_SYSTEM_SIZE);
	  start_with_given_configuration = false;
	  have_configuration = false;
	}
      else
	{
	  read_permanent_data_file( argv[2]);
	  start_with_given_configuration = true;
	  have_configuration = true;
	}
    }
  else
    {
      // initialize empty EEPROM data file
      memset ((uint8_t*) permanent_data_file_storage, 0xff,
    	  EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
      permanent_data_file.set_memory_to_existing_data (
    	  (uint32_t*) permanent_data_file_storage,
    	  (uint32_t*) permanent_data_file_storage + EEPROM_FILE_SYSTEM_SIZE);
      start_with_given_configuration = false;
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

  legacy_observations_type f37_data;
  f37_data.c.position = float3vector();
  ofstream f37_file;
  if( write_f37)
    {
      char * dot = strrchr( buf, '.');
      if( dot == 0)
	exit(0);
      dot[1] = 0;
      strcat( buf, "f37");
      f37_file.open( buf, ios::out | ios::binary | ios::ate);
      if( not f37_file.is_open ())
        {
          cout << "Unable to open output file\n";
          return -1;
        }
    }

  ensure_EEPROM_parameter_integrity();

  uint32_t * in_data = new uint32_t[MAX_SUPPORTED_RECORD_SIZE_WORDS];
  unsigned records = 0;
  uint32_t next_block_identifier_read;
  uint32_t next_block_identifier;

  organizer_t *organizer = 0;
  bool success;
  bool measurement_initialized = false;
  unsigned counter_10Hz=0;
  bool need_to_dump_EEPROM_data = true;
  bool have_basic_sensor_data = false;
  unsigned record_count_100Hz = 0;
  unsigned records_out = 0;
  unsigned x_mag_records = 0;
  unsigned GNSS_sample_on_takeoff = 0;
  unsigned GNSS_sample_number = 0;
  system_state = fake_system_state;

// read until a configuration file is found
  while (in_file.read ((char*) &next_block_identifier_read,
		       sizeof(next_block_identifier_read)))
    {
      int size = flexible_log_file_t::verify_record_get_size (
	  next_block_identifier_read);

      if (size == 0) // error, format not recognized
	{
	  printf ("\nBAD RECORD, WRONG CRC, id=%02x\n",
		  next_block_identifier_read & 0xff);
	  continue;
	}

      if ((next_block_identifier & 0xffff) == 0xffff)
	{
	  uint32_t extended_header[2]; // 32bit identifier and 32bit length
	  in_file.read ((char*) extended_header, sizeof(extended_header));
	  size = flexible_log_file_t::verify_extended_record_get_size (
	      next_block_identifier, extended_header[0], extended_header[1]);
	  if (size == 0)
	    {
	      printf ("\nBAD EXTENDED RECORD %02x\n",
		      next_block_identifier & 0xff);
	      break; // error
	    }
	  next_block_identifier = extended_header[0]; // replace short ID by extended ID
	}
      else
	next_block_identifier = next_block_identifier_read & 0xff; // keep only the ID

      if (size > MAX_SUPPORTED_RECORD_SIZE_WORDS)
	break;

      in_file.read ((char*) in_data, size * sizeof(uint32_t));
      unsigned bytes_read = in_file.gcount ();
      if (bytes_read != (size * sizeof(uint32_t)))
	break;

      if (size == 0) // error, format not recognized
	{
	  printf ("\nBAD RECORD, WRONG CRC, id=%02x\n",
		  next_block_identifier_read & 0xff);
	  continue;
	}

      if (next_block_identifier == EEPROM_FILE && not start_with_given_configuration)
	{
	  memset ((uint8_t*) permanent_data_file_storage, 0xff,
	  EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
	  memcpy (permanent_data_file_storage, in_data, bytes_read);
	  permanent_data_file.set_free_space ();
	  permanent_data_file.setup_registry ();
	  success = permanent_data_file.is_consistent ();
	  if (not success)
	    {
	      cout << "EEPROM data entry not consistent\n";
	      return -1;
	    }

	  have_configuration = true;
	}

      if ( GNSS_sample_on_takeoff == 0) // did not find takeoff by now
	{
	  if (next_block_identifier == D_GNSS_DATA)
	    {
	      ++GNSS_sample_number;
	      assert(size * sizeof(uint32_t) == sizeof(D_GNSS_coordinates_t));
	      memcpy ((uint8_t*) &(coordinates), in_data, size * sizeof(uint32_t));
	      if (coordinates.velocity.abs () > 50.0 / 3.6)
		GNSS_sample_on_takeoff = GNSS_sample_number;
	    }
// ***********************************************************************************************************
	  if (next_block_identifier == GNSS_DATA)
	    {
	      ++GNSS_sample_number;
	      assert(size * sizeof(uint32_t) == sizeof(GNSS_coordinates_t));
	      memcpy ((uint8_t*) &(coordinates), in_data, size * sizeof(uint32_t));
	      if (coordinates.velocity.abs () > 50.0 / 3.6)
		GNSS_sample_on_takeoff = GNSS_sample_number;
	    }
	}
    }

  // now read all data from the very beginning
  in_file.clear();
  in_file.seekg (0, ios::beg);

  GNSS_sample_number = 0;
  if( GNSS_sample_on_takeoff > 600)
    GNSS_sample_on_takeoff -= 600;
  else
    GNSS_sample_on_takeoff = 1;

  while ( in_file.read (
      (char*) &next_block_identifier_read,
      sizeof(next_block_identifier_read)))
    {
      int size = flexible_log_file_t::verify_record_get_size ( next_block_identifier_read);

      if (size == 0) // error, format not recognized
	{
	  printf( "\nBAD RECORD, WRONG CRC, id=%02x\n", next_block_identifier_read & 0xff);
	  continue;
	}

      if( (next_block_identifier & 0xffff) == 0xffff)
	{
	  uint32_t extended_header[2]; // 32bit identifier and 32bit length
	  in_file.read ((char*) extended_header, sizeof( extended_header));
	  size = flexible_log_file_t::verify_extended_record_get_size ( next_block_identifier, extended_header[0], extended_header[1]);
	  if( size == 0)
	    {
	      printf( "\nBAD EXTENDED RECORD %02x\n", next_block_identifier &0xff);
	      break; // error
	    }
	  next_block_identifier = extended_header[0]; // replace short ID by extended ID
	}
      else
	next_block_identifier =   next_block_identifier_read & 0xff; // keep only the ID

      if( size > MAX_SUPPORTED_RECORD_SIZE_WORDS)
	break;

      in_file.read ((char*) in_data, size * sizeof(uint32_t));
      unsigned bytes_read = in_file.gcount ();
      if (bytes_read != (size * sizeof(uint32_t)))
	break;

#if 0 // monitor advance
      static unsigned linecount = 0;
      printf( "%02x ", next_block_identifier);
      ++linecount;
      if( linecount == 50)
	{
	  linecount = 0;
	  printf( "\n");
	}
#endif

      switch ( next_block_identifier)
	{
// ***********************************************************************************************************
	case FLIGHT_EVENT:
	  {
	  const char * event_name[] =
	      {
		  "NO_EVENT",

		  "MAG_CALIBRATION_DONE",
		  "EXT_MAG_CALIBRATION_DONE",
		  "AIR_DENSITY_MODIFIED",

		  "EEPROM_CONFIGURATION_CHANGED",
		  "CAN_COMMAND_RECEIVED",

		  "DEBUGGER_EVENT",
		  "ACCELERATION_CALIBRATION_DONE"
	      };
	  const char * communicator_command[] =
	      {
		  "NO_COMMAND",
		  "MEASURE_CALIB_LEFT",
		  "MEASURE_CALIB_RIGHT",
		  "MEASURE_CALIB_LEVEL",
		  "SET_SENSOR_ROTATION",
		  "FINE_TUNE_CALIB",
		  "SOME_EEPROM_VALUE_HAS_CHANGED"
	      };

	  if( (size != 1) || ((*in_data)&0xff) > (sizeof(event_name)/sizeof(char *)) )
	    {
		printf("Event: %08x size = %d INVALID\n", *in_data, size);
	    }
	  else
	    {
	      if( ((*in_data)&0xff) == CAN_COMMAND_RECEIVED)
		{
		  printf("Event: %s : %s\n", event_name[(*in_data)&0xff], communicator_command[(*in_data) >> 8]);
		  communicator_command_t command = (communicator_command_t)((*in_data) >> 8);
		  organizer->on_command( command, coordinates, observations);
		}
	      else
		printf("Event: %s %08x\n", event_name[(*in_data)&0xff], (*in_data) >> 8);
	    }
	  }
	  break;
// ***********************************************************************************************************
	case EEPROM_FILE:
	  if( start_with_given_configuration)
	    break;

	  memset ((uint8_t*) permanent_data_file_storage, 0xff,
		  EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
	  memcpy (permanent_data_file_storage, in_data, bytes_read);
	  permanent_data_file.set_free_space();
	  permanent_data_file.setup_registry();
	  success = permanent_data_file.is_consistent();
	  if (not success)
	    {
	      cout << "EEPROM data entry not consistent\n";
	      return -1;
	    }

	  cout << "EEPROM data read:\n";
	  permanent_data_file.dump_all_entries();
	  need_to_dump_EEPROM_data = false;

	  have_configuration = true;
	  break;
// ***********************************************************************************************************
	case EEPROM_FILE_RECORD:
	  {
	    if( start_with_given_configuration)
		break;

	    EEPROM_file_system_node *candidate = (EEPROM_file_system_node *)in_data;
	    if( not EEPROM_file_system<LOWEST_UNUSED_EEPROM_ID>::node_is_consistent( candidate))
		{
		  printf(" EEPROM data file (id = %d sz= %d) not consistent\n", candidate->id, candidate->size);
		  return -1;
		}
	    EEPROM_file_system_node::ID_t id = candidate->id;
	    float32_t data = *(float32_t *)(in_data+1);

	    permanent_data_file.store_data( id, candidate->size - 1, (void *)(candidate+1));
	    printf( "id= %d val = %e\n", id, *(float*)(candidate+1));

	    if( id == 42)
	      have_configuration = true; // todo patch kind of too simple ...
	  }
	  break;
// ***********************************************************************************************************
	case LARUS_DESCRIPTION:
	  {
	    printf("Firmware: %s\n", in_data);
	    printf("Larus ID:\n%08x%08x%08x%08x\n", in_data[8], in_data[9], in_data[10], in_data[11]);
	    printf("Flash SHA256:\n%08x%08x%08x%08x\n%08x%08x%08x%08x\n",
		   in_data[12], in_data[13], in_data[14], in_data[15], in_data[16], in_data[17], in_data[18], in_data[19]);
	  }
	  break;
// ***********************************************************************************************************
	case BASIC_SENSOR_DATA:
	  {
	  ++record_count_100Hz;
	  have_basic_sensor_data = true;
	  if( need_to_dump_EEPROM_data)
	    {
	      need_to_dump_EEPROM_data = false;
	      cout << "EEPROM data read:\n";
	      permanent_data_file.dump_all_entries();
	    }

	  ++records;

	  assert( size * sizeof(uint32_t) == sizeof( measurement_data_t));
	  memcpy( (uint8_t *)&observations, in_data, size * sizeof(uint32_t));

	  if( measurement_initialized)
	    {
	      organizer->on_new_pressure_data( observations.static_pressure, observations.pitot_pressure);
	      organizer->update_at_100_Hz( observations, system_state, external_induction);
	    }
	  if( ++counter_10Hz == 10)
	    {
	      counter_10Hz = 0;
	      if( measurement_initialized)
		{
		    bool landing_detected = organizer->update_at_10Hz ( coordinates, observations);

		    if (landing_detected)
		      {
			organizer->cleanup_after_landing();
			printf ("landed at log time %d minutes.\n", records / 6000);
		      }
		}
	    }

	  if(organizer != 0)
	      {
	      bool significant_configuration_change = organizer->manage_attitude_setup_in_progress( coordinates, observations);

	      if( significant_configuration_change)
		{
		  printf("Significant configuration change\n");
		}
	      }

	  if( (organizer != 0) && (GNSS_sample_number > GNSS_sample_on_takeoff)) // after initialization
	    {
	      organizer->report_data ( state_vector);
	      out_file.write ( (const char*)&observations, sizeof( observations));
	      out_file.write ( (const char*)&external_induction, sizeof( external_induction));
	      out_file.write ( (const char*)&coordinates, sizeof(coordinates));
	      out_file.write ( (const char*)&system_state, sizeof(system_state));
	      out_file.write ( (const char*)&state_vector, sizeof(state_vector_t));
	      ++records_out;

	      if( write_f37)
		{
		  f37_data.m = observations;
		  f37_data.c.speed = coordinates.velocity;
		  f37_data.c.relPosNED = coordinates.relPosNED;
		  f37_data.c.relPosHeading = coordinates.relPosHeading;
		  f37_data.c.speed_acc = coordinates.speed_acc;
		  f37_data.c.latitude = coordinates.latitude;
		  f37_data.c.longitude = coordinates.longitude;
		  f37_data.c.position[DOWN] = -coordinates.GNSS_MSL_altitude;

		  f37_data.c.year = coordinates.year;
		  f37_data.c.month = coordinates.month;
		  f37_data.c.day = coordinates.day;
		  f37_data.c.hour = coordinates.hour;
		  f37_data.c.minute = coordinates.minute;
		  f37_data.c.second = coordinates.second;

		  f37_data.c.SATS_number = coordinates.SATS_number;
		  f37_data.c.sat_fix_type = coordinates.sat_fix_type;
		  f37_data.c.second = coordinates.second;
		  f37_data.c.nano = coordinates.nano;
		  f37_data.c.geo_sep_dm = coordinates.geo_sep_dm;

		  f37_file.write ( (const char*)&f37_data, sizeof( legacy_observations_type));
		}
	    }
	}
	  break;
// ***********************************************************************************************************
	case MAGNETOMETER_DATA:
	  ++x_mag_records;
	  assert( size * sizeof(uint32_t) == sizeof( float3vector));
	  memcpy( (uint8_t *)&(external_induction), in_data, size * sizeof(uint32_t));
#if 1
	  system_state |= EXTERNAL_MAGNETOMETER_AVAILABLE;
#else
	  system_state &= ~EXTERNAL_MAGNETOMETER_AVAILABLE;
#endif
	  break;
// ***********************************************************************************************************
	  case D_GNSS_DATA:
	    {
	    ++GNSS_sample_number;
	    assert( size * sizeof(uint32_t) == sizeof( D_GNSS_coordinates_t));
	    memcpy( (uint8_t *)&( coordinates), in_data, size * sizeof(uint32_t));

#if PRINT_GNSS_RATE
	      static int old;
	      float delta = state_vector.observations.c.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = state_vector.observations.c.nano;
#endif

	      if( have_basic_sensor_data)
		{
		if( not measurement_initialized && have_configuration)
		  {
		    organizer = new organizer_t;

		    organizer->initialize_before_measurement ();
		    organizer->initialize_after_first_measurement ( coordinates, observations);
		    organizer->update_magnetic_induction_data( coordinates.latitude, coordinates.longitude);

		    measurement_initialized = true;
		  }
		}

	      if( organizer)
		organizer->update_GNSS_data ( coordinates);

	      state_vector.satfix = coordinates.sat_fix_type;
	    }
	    break;
// ***********************************************************************************************************
	    case GNSS_DATA:
	      {
		++GNSS_sample_number;
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

	      if( have_basic_sensor_data)
		{
		if( not measurement_initialized && have_configuration)
		  {
		    organizer = new organizer_t;

		    organizer->initialize_before_measurement ();
		    organizer->initialize_after_first_measurement ( coordinates, observations);
		    organizer->update_magnetic_induction_data( coordinates.latitude, coordinates.longitude);

		    measurement_initialized = true;
		  }

		if( organizer)
		  organizer->update_GNSS_data ( coordinates);

		state_vector.satfix = coordinates.sat_fix_type;
		}
	      break;
	      }
// ***********************************************************************************************************
	case SENSOR_STATUS:
	  assert( size == 1);
	  system_state = *in_data;
	  break;
	}
    }

  if( organizer)
    organizer->cleanup_after_landing(); // latest here

  printf ("\n%d records in, %d ext_mag_records,  %d records out\n", records, x_mag_records, records_out);
  out_file.close ();

  if( write_f37)
    f37_file.close();

  strcpy( buf, argv[1]);
  char * p = strrchr( buf, '/');
  if( p == 0)
    exit(1);
  *p=0;
  write_EEPROM_dump( buf);
  write_permanent_data_file( argv[1]);

  exit( 0);
}
