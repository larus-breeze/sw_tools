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
#include "navigator.h"
#include "NMEA_format.h"
#include <fenv.h>
#include "CAN_output.h"
#include "NMEA_format.h"
#include "TCP_server.h"
#include "USB_serial.h"

using namespace std;

int
read_identifier (char *s)
{
  if (s[2] != ' ')
    return EEPROM_PARAMETER_ID_END;
  int identifier = atoi (s);
  if ((identifier > 0) && (identifier < EEPROM_PARAMETER_ID_END))
    return identifier;
  return EEPROM_PARAMETER_ID_END; // error
}

int
read_EEPROM_file (char *basename)
{
  char buf[50];
  strcpy (buf, basename);
  strcat (buf, ".EEPROM");

  FILE *fp = fopen (buf, "r");
  if (fp == NULL)
    exit (EXIT_FAILURE);

  char *line = NULL;
  size_t len = 0;
  while ((getline (&line, &len, fp)) != -1)
    {
      EEPROM_PARAMETER_ID identifier = (EEPROM_PARAMETER_ID) read_identifier (
	  line);
      if (identifier == EEPROM_PARAMETER_ID_END)
	continue;
      const persistent_data_t *param = find_parameter_from_ID (identifier);
      unsigned name_len = strlen (param->mnemonic);
      if (0
	  != strncmp ((const char*) (param->mnemonic), (const char*) (line + 3),
		      name_len))
	continue;
      if (line[name_len + 4] != '=')
	continue;
      float value = atof (line + name_len + 6);

      config_parameters[identifier].identifier = identifier;
      config_parameters[identifier].value = value;
    }
  fclose (fp);
  if (line)
    free (line);

}

int main (int argc, char *argv[])
{
  unsigned init_counter=10000;
  unsigned skiptime;
  float declination; // todo fixme this variable is somewhat misplaced here

  //  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
  feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

  if ((argc != 2) && (argc != 3))
    {
      printf ("usage: %s infile.f50 [skiptime for TCP version]\n", argv[0]);
      return -1;
    }

  bool realtime_with_TCP_server = argc == 3;
  if( realtime_with_TCP_server)
    skiptime = 10 * atoi( argv[2]); // 10Hz output rate

  input_data_t *in_data;
  output_data_t *output_data;

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
      USB_active = open_USB_serial ();
      realtime_with_TCP_server = wait_and_accept_TCP_connection();
    }

  read_EEPROM_file (argv[1]);

  navigator_t navigator;
  float3vector acc, mag, gyro;

  streampos size = file.tellg ();
  in_data = (input_data_t*) new char[size];

  unsigned records = size / sizeof(input_data_t);
  size_t outfile_size = records * sizeof(output_data_t);
  output_data = (output_data_t*) new char[outfile_size];

  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();

// ************************************************************

  records = 0;

  float3matrix sensor_mapping;
    {
      quaternion<float> q;
      q.from_euler (configuration (SENS_TILT_ROLL),
		    configuration (SENS_TILT_NICK),
		    configuration (SENS_TILT_YAW));
      q.get_rotation_matrix (sensor_mapping);
    }

  float pitot_offset = configuration (PITOT_OFFSET);
  float pitot_span   = configuration (PITOT_SPAN);
  float QNH_offset   = configuration (QNH_OFFSET);

  declination = navigator.get_declination();

  navigator.update_pressure_and_altitude(in_data[0].m.static_pressure - QNH_offset, -in_data[0].c.position[DOWN]);
  navigator.initialize_QFF_density_metering( -in_data[0].c.position[DOWN]);
  navigator.reset_altitude ();

  // setup initial attitude
  acc = sensor_mapping * in_data->m.acc;
  mag = sensor_mapping * in_data->m.mag;
  navigator.set_from_add_mag (acc, mag); // initialize attitude from acceleration + compass

  int32_t nano = 0;
  unsigned fife_hertz_counter = 0;

  int delta_time;

  for (unsigned count = 0; count < size / sizeof(input_data_t); ++count)
    {
      if( init_counter)
	--init_counter;

      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;

      navigator.update_pressure_and_altitude(output_data[count].m.static_pressure - QNH_offset, -in_data[count].c.position[DOWN]);
      navigator.update_pitot ( (output_data[count].m.pitot_pressure - pitot_offset) * pitot_span);

      if (output_data[count].c.nano != nano) // 10 Hz by GNSS
	{
	  delta_time = output_data[count].c.nano - nano;
	  if( delta_time < 0 )
	    delta_time += 1000000000;
	  nano = output_data[count].c.nano;
	  navigator.update_GNSS (output_data[count].c);
	  navigator.feed_QFF_density_metering( output_data[count].m.static_pressure - QNH_offset, -in_data[count].c.position[DOWN]);
	}

      // rotate sensor coordinates into airframe coordinates
      acc =  sensor_mapping * output_data[count].m.acc;
      mag =  sensor_mapping * output_data[count].m.mag;
      gyro = sensor_mapping * output_data[count].m.gyro;

      output_data[count].body_acc  = acc;
      output_data[count].body_gyro = gyro;

      navigator.update_IMU (acc, mag, gyro);
      navigator.report_data (output_data[count]);

      if( count % 10 == 0)
	{
//	CAN_output ( (const output_data_t&) *(output_data+count));
	  if( realtime_with_TCP_server)
	    {
	      if( skiptime > 0)
		--skiptime;
	      else
		{
		  NMEA_buffer_t buffer;
		  format_NMEA_string( (const output_data_t&) *(output_data+count), buffer, declination);
		  write_TCP_port( buffer.string, buffer.length);

		  if( USB_active)
		      CAN_output( (const output_data_t&) *(output_data+count));

		  timespec want,got;
		  want.tv_nsec = 100000000;
		  want.tv_sec = 0;
		  while( nanosleep( &want, &got))
		    want = got;
		}
	    }
	}
      ++records;
    }
  printf ("%d records\n", records);

  char buf[50];
  char ascii_len[10];
  sprintf (ascii_len, "%d", sizeof(output_data_t) / sizeof(float));
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
