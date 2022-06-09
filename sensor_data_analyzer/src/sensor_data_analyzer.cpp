#include <iostream>
#include <fstream>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "data_structures.h"
#include "persistent_data.h"
#include "EEPROM_emulation.h"
#include "navigator.h"
#include <fenv.h>

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

int
main (int argc, char *argv[])
{
  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

  if (argc != 2)
    {
      printf ("usage: %s infile %s\n", argv[0], argv[1]);
      return -1;
    }

  volatile unsigned size1 = sizeof(measurement_data_t) / 4;
  volatile unsigned size2 = sizeof(coordinates_t) / 4;
  volatile unsigned size3 = sizeof(output_data_t) / 4;

  streampos size;

  input_data_t *in_data;
  output_data_t *output_data;

#if 0
  ofstream outfile ( argv[2], ios::out|ios::binary);
  if (! outfile.is_open())
	  return -1;
#endif

  ifstream file (argv[1], ios::in | ios::binary | ios::ate);
  if (!file.is_open ())
    {
      cout << "Unable to open file";
      return -1;
    }

  read_EEPROM_file (argv[1]);

  navigator_t navigator;
  float3vector acc, mag, gyro;

  size = file.tellg ();
  in_data = (input_data_t*) new char[size];

  unsigned records = size / sizeof(input_data_t);
  size_t outfile_size = records * sizeof(output_data_t);
  output_data = (output_data_t*) new char[outfile_size];

  file.seekg (0, ios::beg);
  file.read ((char*) in_data, size);
  file.close ();

// ************************************************************

  records = 0;
  unsigned errors = 0;

  float3matrix sensor_mapping;
    {
      quaternion<float> q;
      q.from_euler (configuration (SENS_TILT_ROLL),
		    configuration (SENS_TILT_NICK),
		    configuration (SENS_TILT_YAW));
      q.get_rotation_matrix (sensor_mapping);
    }

  float pitot_offset = configuration (PITOT_OFFSET);
  float pitot_span = configuration (PITOT_SPAN);
  float QNH_offset = configuration (QNH_OFFSET);

  navigator.update_pabs (in_data->m.static_pressure);
  navigator.reset_altitude ();

  // setup initial attitude
  acc = sensor_mapping * in_data->m.acc;
  mag = sensor_mapping * in_data->m.mag;
  navigator.set_from_add_mag (acc, mag); // initialize attitude from acceleration + compass

  int32_t nano = 0;

  for (unsigned count = 0; count < size / sizeof(input_data_t); ++count)
    {
      output_data[count].m = in_data[count].m;
      output_data[count].c = in_data[count].c;

      navigator.update_pabs (output_data[count].m.static_pressure - QNH_offset);
      navigator.update_pitot (
	  (output_data[count].m.pitot_pressure - pitot_offset) * pitot_span);

      if (output_data[count].c.nano != nano) // 10 Hz by GNSS
	{
	  nano = output_data[count].c.nano;
	  navigator.update_GNSS (output_data[count].c);
	}

      // rotate sensor coordinates into airframe coordinates
      acc = sensor_mapping * output_data[count].m.acc;
      mag = sensor_mapping * output_data[count].m.mag;
      gyro = sensor_mapping * output_data[count].m.gyro;

      navigator.update_IMU (acc, mag, gyro);
      navigator.report_data (output_data[count]);

      ++records;
    }
  printf ("%d records\n", records);

  char buf[50];
  char ascii_len[10];
  sprintf (ascii_len, "%d", sizeof(output_data_t) / sizeof(float));
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

  delete[] in_data;
  delete[] output_data;
}
