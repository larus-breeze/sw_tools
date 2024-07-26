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
#include "CAN_socket_driver.h"
#include "CAN_gateway.h"
#include "math.h"
#include "random"

using namespace std;

uint32_t system_state // fake system state here in lack of hardware
  = GNSS_AVAILABLE | MTI_SENSOR_AVAILABE | MS5611_STATIC_AVAILABLE | PITOT_SENSOR_AVAILABLE;

#define N_TWEAKS 6
#define N_TRIALS 50
#define TRIAL_STEPSIZE 0.003

double randn()
{
  // Standard normal random variable
  static std::default_random_engine rn_generator = std::default_random_engine();
  static std::normal_distribution<double> standard_normal_dist{0.0, 1.0};

  return standard_normal_dist(rn_generator);
}

int
main (int argc, char *argv[])
{
  bool do_optimize = false;
  unsigned start_count;
  unsigned stop_count;

#ifndef _WIN32
  //  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
  // don't enable UNDERFLOW as this can happen regularly when filter outputs decay
  feenableexcept ( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  if (argc == 2)
    {
      do_optimize = false;
      stop_count = 1;
    }
  else if (argc != 4)
    {
      printf ("usage: %s infile.f37 count_start count_stop\n", argv[0]);
      return -1;
    }
  else
    {
      start_count = atoi (argv[2]);
      stop_count = atoi (argv[3]);
      do_optimize = true;
    }

  output_data_t *output_data;

  ifstream file (argv[1], ios::in | ios::binary | ios::ate);
  if (!file.is_open ())
    {
      cout << "Unable to open file";
      return -1;
    }

  // cut off file extension
  char basename[100];
  strcpy (basename, argv[1]);
  char *dot = strchr (basename, '.');
  if ((dot != 0) && (dot[1] == 'f')) // old format: filename.f37.EEPROM new: filename.EEPROM
    *dot = 0; // cut off .f37 extension

// try to read "config.EEPROM" first
  char config_path[200];
  strcpy (config_path, basename);
  char *slash_location = strrchr (config_path, '/');
  *slash_location = 0;
  strcat (config_path, "/config");

  if (read_EEPROM_file (config_path) == EXIT_FAILURE)
    {
      // try to read the EEPROM file accompanying the data file
      if (read_EEPROM_file (basename) == EXIT_FAILURE)
	{
	  cout << "Unable to open EEPROM file";
	  return -1;
	}
    }

  ensure_EEPROM_parameter_integrity ();

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

  output_data[0].m = in_data[0].m;
  output_data[0].c = in_data[0].c;

  organizer.initialize_after_first_measurement (output_data[0]);
  organizer.update_GNSS_data (output_data[0].c);
  organizer.update_magnetic_induction_data (output_data[0].c.latitude,
					    output_data[0].c.longitude);

  bool have_GNSS_fix = false;

  int32_t nano = 0;
  int delta_time;
  unsigned counter_10Hz = 10;

  float tweaks[N_TWEAKS] =
    { 0.0f };
  if (do_optimize)
    {
      // run algorithms until test sequence starts
      for (unsigned count = 1; count < start_count; ++count)
	{
	  output_data[count].m = in_data[count].m;
	  output_data[count].c = in_data[count].c;
	  organizer.on_new_pressure_data (output_data[count]);

	  if (have_GNSS_fix == false)
	    {
	      if (output_data[count].c.sat_fix_type > 0)
		{
		  organizer.update_magnetic_induction_data (
		      output_data[count].c.latitude,
		      output_data[count].c.longitude);
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

	  organizer.update_every_10ms (output_data[count], tweaks);

	  --counter_10Hz;
	  if (counter_10Hz == 0)
	    {
	      organizer.update_every_100ms (output_data[count]);
	      counter_10Hz = 10;
	    }
	  organizer.report_data (output_data[count]);
	}

      double average_error = 0.0f;
      unsigned error_count = 0;

      organizer_t start_organizer = organizer;

      // run algorithms through segment of interest
      // to initialize the quality indicator
      for (unsigned count = start_count; count < stop_count; ++count)
	{
	  output_data[count].m = in_data[count].m;
	  output_data[count].c = in_data[count].c;
	  organizer.on_new_pressure_data (output_data[count]);

	  if (have_GNSS_fix == false)
	    {
	      if (output_data[count].c.sat_fix_type > 0)
		{
		  organizer.update_magnetic_induction_data (
		      output_data[count].c.latitude,
		      output_data[count].c.longitude);
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

	  organizer.update_every_10ms (output_data[count], tweaks);

	  --counter_10Hz;
	  if (counter_10Hz == 0)
	    {
	      organizer.update_every_100ms (output_data[count]);
	      counter_10Hz = 10;
	    }
	  organizer.report_data (output_data[count]);

	  average_error += output_data[count].gyro_correction_power;
//	  average_error += output_data[count].magnetic_disturbance;
	  ++error_count;
	}

      double reference_error = average_error / error_count;
      printf ("%e\n", reference_error);

      for (unsigned trial = 0; trial < N_TRIALS; ++trial)
	{
	  for (unsigned try_this = 0; try_this < N_TWEAKS; ++try_this)
	    {
	      organizer = start_organizer;

	      float old_tweak = tweaks[try_this];
	      tweaks[try_this] += (randn () * TRIAL_STEPSIZE);
	      average_error = 0.0;
	      error_count = 0;

	      // run algorithms through segment of interest
	      for (unsigned count = start_count; count < stop_count; ++count)
		{
		  output_data[count].m = in_data[count].m;
		  output_data[count].c = in_data[count].c;
		  organizer.on_new_pressure_data (output_data[count]);

		  if (have_GNSS_fix == false)
		    {
		      if (output_data[count].c.sat_fix_type > 0)
			{
			  organizer.update_magnetic_induction_data (
			      output_data[count].c.latitude,
			      output_data[count].c.longitude);
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

		  organizer.update_every_10ms (output_data[count], tweaks);

		  --counter_10Hz;
		  if (counter_10Hz == 0)
		    {
		      organizer.update_every_100ms (output_data[count]);
		      counter_10Hz = 10;
		    }
		  organizer.report_data (output_data[count]);

		  average_error += output_data[count].gyro_correction_power;
	//	  average_error += output_data[count].magnetic_disturbance;
		  ++error_count;
		}

	      double observed_error = average_error / error_count;
	      if (observed_error < reference_error)
		{
		  reference_error = observed_error;
		  printf ("%e ", reference_error);
		  for (unsigned i = 0; i < N_TWEAKS; ++i)
		    printf ("%f ", tweaks[i]);

		  printf ("\n");
		}
	      else
		tweaks[try_this] = old_tweak; // keep old setting
	    }
	}

#if 0 // many parameters optimized
  for (unsigned trial = 0; trial < 100; ++trial)
    {
      for (unsigned try_this = 0; try_this < N_TWEAKS; ++try_this)
	{
	  float old_tweak = tweaks[try_this];
	  tweaks[try_this] += randn () * 0.01f;
	  average_error = 0.0;
	  error_count = 0;

	  // run algorithms through segment of interest
	  for (unsigned count = start_count; count < stop_count; ++count)
	    {
	      output_data[count].m = in_data[count].m;
	      output_data[count].c = in_data[count].c;
	      organizer.on_new_pressure_data (output_data[count]);

	      if (have_GNSS_fix == false)
		{
		  if (output_data[count].c.sat_fix_type > 0)
		    {
		      organizer.update_magnetic_induction_data (
			  output_data[count].c.latitude,
			  output_data[count].c.longitude);
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

	      organizer.update_every_10ms (output_data[count], tweaks);

	      --counter_10Hz;
	      if (counter_10Hz == 0)
		{
		  organizer.update_every_100ms (output_data[count]);
		  counter_10Hz = 10;
		}
	      organizer.report_data (output_data[count]);

	      average_error += output_data[count].magnetic_disturbance;
	      ++error_count;
	    }

	  double observed_error = average_error / error_count;
	  if (observed_error < reference_error)
	    reference_error = observed_error;
	  else
	    tweaks[try_this] = old_tweak; // keep old setting

	  printf ("%f ", reference_error);
	  for (unsigned i = 0; i < N_TWEAKS; ++i)
	    printf ("%f ", tweaks[i]);

	  printf ("\r");
	}
#endif
      // run algorithms a last time through segment of interest with all parameters optimized
      for (unsigned count = start_count; count < stop_count; ++count)
	{
	  output_data[count].m = in_data[count].m;
	  output_data[count].c = in_data[count].c;
	  organizer.on_new_pressure_data (output_data[count]);

	  if (have_GNSS_fix == false)
	    {
	      if (output_data[count].c.sat_fix_type > 0)
		{
		  organizer.update_magnetic_induction_data (
		      output_data[count].c.latitude,
		      output_data[count].c.longitude);
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

	  organizer.update_every_10ms (output_data[count], tweaks);

	  --counter_10Hz;
	  if (counter_10Hz == 0)
	    {
	      organizer.update_every_100ms (output_data[count]);
	      counter_10Hz = 10;
	    }
	  organizer.report_data (output_data[count]);

	  average_error += output_data[count].gyro_correction_power;
	  ++error_count;
	}
    }
  // run algorithms until end of data
  for (unsigned count = stop_count; count < records; ++count)
    {
      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;
      organizer.on_new_pressure_data (output_data[count]);

      if (have_GNSS_fix == false)
	{
	  if (output_data[count].c.sat_fix_type > 0)
	    {
	      organizer.update_magnetic_induction_data (
		  output_data[count].c.latitude,
		  output_data[count].c.longitude);
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

      organizer.update_every_10ms (output_data[count], tweaks);

      --counter_10Hz;
      if (counter_10Hz == 0)
	{
	  organizer.update_every_100ms (output_data[count]);
	  counter_10Hz = 10;
	}
      organizer.report_data (output_data[count]);
    }

  char buf[200];
  char ascii_len[10];
  sprintf (ascii_len, "%d", (int) (sizeof(output_data_t) / sizeof(float)));
  strcpy (buf, argv[1]);
  strcat (buf, ".f");
  strcat (buf, ascii_len);

  ofstream outfile (buf, ios::out | ios::binary | ios::ate);
  if (outfile.is_open ())
    {
      outfile.write ((const char*) output_data,
		     records * sizeof(output_data_t));
      outfile.close ();
    }

  char *path_end = strrchr (buf, '/');
  *path_end = 0;
  write_EEPROM_dump (buf); // make new magnetic data permanent

  delete[] in_data;
  delete[] output_data;
}

void report_magnetic_calibration_has_changed (
    magnetic_induction_report_t *p_magnetic_induction_report, char type)
{
  return;

  magnetic_induction_report_t magnetic_induction_report = *p_magnetic_induction_report;
  char buffer[50];
  char *next = buffer;

  printf (type == 'm' ? "\nMagnetic:\n" : "\nSatellite:\n");

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

#if USE_EARTH_INDUCTION_DATA_COLLECTOR

  float3vector induction = magnetic_induction_report.nav_induction;
  for (unsigned i = 0; i < 3; ++i)
    {
      next = my_ftoa (next, induction[i]);
      *next++ = ' ';
    }

  next = my_ftoa (next, magnetic_induction_report.nav_induction_std_deviation);
  *next++ = 0;

  printf( "Dev=%f Inc=%f\n",
      atan2 (magnetic_induction_report.nav_induction[EAST],
	     magnetic_induction_report.nav_induction[NORTH]) * 180.0 / M_PI,
      atan2 (magnetic_induction_report.nav_induction[DOWN],
	     magnetic_induction_report.nav_induction[NORTH]) * 180.0 / M_PI);

#else
  printf ("\n");
#endif
}

bool CAN_gateway_poll(CANpacket&, unsigned int)
{
  return false; // presently just an empty stub
}
