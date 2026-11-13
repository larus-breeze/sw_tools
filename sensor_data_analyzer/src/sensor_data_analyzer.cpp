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

void print_flight_event_info( uint32_t data, unsigned time);

#define MAX_SUPPORTED_RECORD_SIZE_WORDS 256
#define PRINT_GNSS_RATE 0
#define HONORIZE_EXTERNAL_INDUCTION 1
uint32_t buffer[128];
flexible_log_file_implementation_t flex_file (buffer, 128);

bool
write_block (uint32_t *p_data, uint32_t size_words)
{
  return flex_file.write_block (p_data, size_words);
}

void
signal_logger_event (uint32_t event)
{

}

Mutex_Wrapper_Type my_mutex;

magnetic_calculation_data_t temporary_mag_calculation_data;

compass_calibrator_3D_t compass_calibrator_3D (temporary_mag_calculation_data);
compass_calibrator_3D_t external_compass_calibrator_3D (
    temporary_mag_calculation_data);

void
trigger_compass_calibrator_3D_calculation (bool calculate_external_magnetometer)
{
  if (calculate_external_magnetometer)
    external_compass_calibrator_3D.calculate ();
  else
    compass_calibrator_3D.calculate ();
}

#ifdef _WIN32
# pragma float_control(except, on)
#endif

using namespace std;

uint32_t system_state;
state_vector_t state_vector;
D_GNSS_coordinates_t coordinates;
measurement_data_t observations;
magnetometer_data_timed_t external_induction_timed;
imu_data_timed_t imu_data;
pressure_data_timed_t pressure_record;

uint32_t fake_system_state // fake system state here in lack of hardware
= GNSS_AVAILABLE | MTI_SENSOR_AVAILABE | MS5611_STATIC_AVAILABLE
    | PITOT_SENSOR_AVAILABLE;

uint32_t UNIQUE_ID[4] =
  { 0x4711, 0, 0, 0 };

int
main (int argc, char *argv[])
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

  bool start_with_given_configuration;
  if (argc == 3)
    {
	  read_permanent_data_file (argv[2]);
	  start_with_given_configuration = true;
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
  strcpy (buf, argv[1]);
  strcat (buf, ".f");
    {
      char size_string[10];
      itoa (
	  (sizeof(measurement_data_t) + sizeof(float3vector)
	      + sizeof(D_GNSS_coordinates_t) + sizeof(uint32_t)
	      + sizeof(state_vector_t)) / sizeof(uint32_t)
	      + sizeof(float3vector) / sizeof(uint32_t),
	  size_string, 10);
      strcat (buf, size_string);
    }

  ofstream out_file (buf, ios::out | ios::binary | ios::ate);
  if (not out_file.is_open ())
    {
      cout << "Unable to open output file\n";
      return -1;
    }

  ensure_EEPROM_parameter_integrity ();

  uint32_t *input_buffer = new uint32_t[MAX_SUPPORTED_RECORD_SIZE_WORDS];
  unsigned records = 0;

  organizer_t *organizer = 0;
  bool success;
  unsigned records_out = 0;
  unsigned x_mag_records = 0;
  unsigned GNSS_sample_number = 0;
  system_state = fake_system_state;
  record_header_t record_header;
  unsigned data_size_words;
  unsigned LRSX_file_format_version = 1; // start with default value
  input_data_t new_input;

  while (in_file.read ((char*) input_buffer, sizeof(record_header_t)))
    {
      uint32_t *data_start;

      // read the record header
      unsigned bytes_read = in_file.gcount ();
      if (bytes_read != sizeof(record_header_t))
	break; // EOF ?

      record_header = *(record_header_t*) input_buffer;

      data_start = input_buffer + 1;

      // read the record data
      unsigned data_size_bytes = (record_header.total_size - 1)
	  * sizeof(uint32_t);
      in_file.read ((char*) data_start, data_size_bytes);
      if (in_file.gcount () != data_size_bytes)
	break; // EOF ?

      // verify the record's integrity
      data_size_words = flexible_log_file_t::verify_record_get_size (
	  input_buffer, LRSX_file_format_version);
      if (data_size_words == 0) // error, format not recognized
	{
	  printf ("\nBAD RECORD # %d, WRONG CRC, NODE=%08x \n", records,
		  input_buffer[0]);
	  continue;
	}

#if 0 // monitor advance
      static unsigned linecount = 0;
      printf( "%02x ", record_header.record_type);
      ++linecount;
      if( linecount == 50)
	{
	  linecount = 0;
	  printf( "\n");
	}
#endif

      switch (record_header.record_type)
	{
// ***********************************************************************************************************
	case FLIGHT_EVENT:
	  {
	    switch (data_size_words)
	      {
	      case 1:
		print_flight_event_info (input_buffer[1], records);
		break;
	      case 2:
		print_flight_event_info (input_buffer[1], buffer[2]);
		break;
	      default:
		printf ("\nBAD RECORD # %d, WRONG CRC, NODE=%08x \n", records,
			input_buffer[0]);
		break;
	      }
	  }
	  break;
// ***********************************************************************************************************
	case EEPROM_FILE:
	  {
	    if (start_with_given_configuration) // ignore this record
	      break;

	    memset ((uint8_t*) permanent_data_file_storage, 0xff,
	    EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
	    memcpy (permanent_data_file_storage, (uint8_t*) (input_buffer + 1),
		    data_size_words * sizeof(uint32_t));
	    permanent_data_file.set_free_space ();
	    permanent_data_file.setup_registry ();
	    success = permanent_data_file.is_consistent ();
	    if (not success)
	      {
		cout << "EEPROM data entry not consistent\n";
		return -1;
	      }

	    cout << "EEPROM data read:\n";
	    permanent_data_file.dump_all_entries ();
	  }
	  break;
// ***********************************************************************************************************
	case EEPROM_FILE_RECORD:
	  {
	    if (start_with_given_configuration)
	      break;

	    EEPROM_file_system_node *candidate =
		(EEPROM_file_system_node*) input_buffer;
	    if (not EEPROM_file_system<LOWEST_UNUSED_EEPROM_ID>::node_is_consistent (
		candidate))
	      {
		printf (" EEPROM data file (id = %d sz= %d) not consistent\n",
			candidate->id, candidate->size);
		return -1;
	      }
	    EEPROM_file_system_node::ID_t id = candidate->id;

	    permanent_data_file.store_data (id, candidate->size - 1,
					    (void*) (candidate + 1));
	    printf ("id= %d val = %e\n", id, *(float*) (candidate + 1));

	  }
	  break;
// ***********************************************************************************************************
	case LARUS_DESCRIPTION:
	  {
	    printf ("\nFirmware: %s\n", (char*) (input_buffer + 1));
	    printf ("Larus ID:\n%08x%08x%08x%08x\n", input_buffer[9],
		    input_buffer[10], input_buffer[11], input_buffer[12]);
	    printf ("Flash SHA256:\n%08x%08x%08x%08x\n%08x%08x%08x%08x\n\n",
		    input_buffer[13], input_buffer[14], input_buffer[15],
		    input_buffer[16], input_buffer[17], input_buffer[18],
		    input_buffer[19], input_buffer[20]);
	  }
	  break;
// ***********************************************************************************************************
	case IMU_DATA:
	  {
	    if (data_size_words * sizeof(uint32_t) == sizeof(imu_data))
	      memcpy ((uint8_t*) &imu_data, (uint8_t*) data_start,
		      data_size_words * sizeof(uint32_t));
	    else
	      {
		printf ("\nBAD RECORD, HEAD=%08x\n", input_buffer[0]);
		continue;
	      }

	    observations.acc = imu_data.acc;
	    observations.mag = imu_data.mag;
	    observations.gyro = imu_data.gyro;

	    new_input.type = IMU_DATA;
	    new_input.data = &imu_data;
	    organizer->on_new_information (new_input);

	    ++records;

	    organizer->report_data (state_vector);
	    out_file.write ((const char*) &observations, sizeof(observations));
	    out_file.write ((const char*) &external_induction_timed,
			    sizeof(float3vector));
	    out_file.write ((const char*) &coordinates, sizeof(coordinates));
	    out_file.write ((const char*) &system_state, sizeof(system_state));
	    out_file.write ((const char*) &state_vector,
			    sizeof(state_vector_t));
	    out_file.write ((const char*) &(external_induction_timed.induction),
			    sizeof(float3vector));

	    ++records_out;
	  }
	  break;
// ***********************************************************************************************************
	case SUPPLEMENTARY_DATA:
	  {

	    supplementary_data_t supp_data;
	    if (data_size_words * sizeof(uint32_t)
		== sizeof(supplementary_data_t))
	      memcpy ((uint8_t*) &supp_data, (uint8_t*) data_start,
		      data_size_words * sizeof(uint32_t));
	    else
	      {
		printf ("\nBAD RECORD, HEAD=%08x\n", input_buffer[0]);
		continue;
	      }

	    observations.supply_voltage = supp_data.supply_voltage;
	    observations.static_sensor_temperature =
		supp_data.sensor_temperature;

	    new_input.type = SUPPLEMENTARY_DATA;
	    new_input.data = &supp_data;
	    organizer->on_new_information (new_input);

	  }
	  break;
// ***********************************************************************************************************
	case PRESSURE_DATA:
	  {
	    if (data_size_words * sizeof(uint32_t)
		== sizeof(pressure_data_timed_t))
	      memcpy ((uint8_t*) &pressure_record, (uint8_t*) data_start,
		      data_size_words * sizeof(uint32_t));
	    else
	      {
		printf ("\nBAD RECORD, HEAD=%08x\n", input_buffer[0]);
		continue;
	      }

	    new_input.type = PRESSURE_DATA;
	    new_input.data = &pressure_record;
	    organizer->on_new_information (new_input);

	    observations.static_pressure = pressure_record.absolute_pressure;
	    observations.pitot_pressure = pressure_record.dynamic_pressure;

	  }
	  break;
// ***********************************************************************************************************
	case MAGNETOMETER_DATA:
	  {
	    if ((data_size_words * sizeof(uint32_t) == sizeof(float3vector))
		or (data_size_words * sizeof(uint32_t)
		    == sizeof(external_induction_timed)))
	      {
		// ignore timestamp for the moment
		memcpy ((uint8_t*) &(external_induction_timed),
			(uint8_t*) data_start,
			sizeof(external_induction_timed) * sizeof(uint32_t));
	      }
	    else
	      {
		printf ("\nBAD RECORD, HEAD=%08x\n", input_buffer[0]);
		continue;
	      }

	    new_input.type = MAGNETOMETER_DATA;
	    new_input.data = &external_induction_timed;
	    organizer->on_new_information (new_input);

	    ++x_mag_records;
#if HONORIZE_EXTERNAL_INDUCTION
	    system_state |= EXTERNAL_MAGNETOMETER_AVAILABLE;
#else
	system_state &= ~EXTERNAL_MAGNETOMETER_AVAILABLE;
#endif
	  }
	  break;
// ***********************************************************************************************************
	case D_GNSS_DATA:
	  {
	    ++GNSS_sample_number;
	    assert(
		(record_header.total_size - 1) * sizeof(uint32_t)
		    == sizeof(D_GNSS_coordinates_t));
	    memcpy ((uint8_t*) &(coordinates), (uint8_t*) (input_buffer + 1),
		    data_size_words * sizeof(uint32_t));

#if PRINT_GNSS_RATE
	      static int old;
	      float delta = coordinates.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = coordinates.nano;
#endif

	    new_input.type = D_GNSS_DATA;
	    new_input.data = &coordinates;
	    organizer->on_new_information (new_input);

	    state_vector.satfix = coordinates.sat_fix_type;
	  }
	  break;
// ***********************************************************************************************************
	case GNSS_DATA:
	  {
	    ++GNSS_sample_number;
	    assert(
		(record_header.total_size - 1) * sizeof(uint32_t)
		    == sizeof(GNSS_coordinates_t));

	    memcpy ((uint8_t*) &(coordinates), (uint8_t*) (input_buffer + 1),
		    data_size_words * sizeof(uint32_t));

#if PRINT_GNSS_RATE
	      static int old;

	      float delta = coordinates.nano - old;
	      if( delta < 0)
		delta += 1000000000;

	      printf("%3.6f\n", delta / 1000000);
	      old = coordinates.nano;
#endif

	    new_input.type = GNSS_DATA;
	    new_input.data = &coordinates;
	    organizer->on_new_information (new_input);
	  }
	  break;
// ***********************************************************************************************************
	case SENSOR_STATUS:
	  {
	    assert(record_header.total_size == 2);
	    system_state = *input_buffer;
	    break;
	    default:
	    printf ("\nRECORD IGNORED, HEAD=%08x\n", input_buffer[0]);
	    break;
	  }
	}
    }

  printf ("\n%d records in, %d ext_mag_records,  %d records out\n", records,
	  x_mag_records, records_out);
  out_file.close ();

  strcpy (buf, argv[1]);
  char *p = strrchr (buf, '/');
  if (p == 0)
    exit (1);
  *p = 0;
  write_EEPROM_dump (buf);
  write_permanent_data_file (argv[1]);

  exit (0);
}
