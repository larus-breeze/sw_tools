#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "persistent_data.h"
#include "persistent_data_file.h"
#include "persistent_data_file_emulation.h"
#include "EEPROM_emulation.h"
#include "data_structures.h"
#include "persistent_data.h"
#include "sensor_orientation_setup.h"

using namespace std;

extern EEPROM_file_system <LOWEST_UNUSED_EEPROM_ID> permanent_data_file;

bool read_meta_data_file (char *file_path)
{
  char path[200];
  strcpy (path, file_path);
  char *slash = strrchr (path, '/');
  if (slash != 0)
    slash[1] = 0;
  else
    {
      printf ("Wrong file path - exiting\n");
      exit (-1);
    }

  slash = strrchr (path, '/');
  if (slash != 0)
    slash[1] = 0;
  else
    {
      printf ("Unable to open configuration - exiting");
      exit (-1);
    }
  strcat (path, "config");
  if (read_EEPROM_file (path) != EXIT_SUCCESS)
    {
      // THIRD preference: Read the meta-data accompanying the *.f32 i.e. "*.EEPROM"
      strcpy (path, file_path);
      char *dot = strrchr (path, '.');
      if (dot)
	*dot = 0;
      if (read_EEPROM_file (path) != EXIT_SUCCESS)
	{
	  printf ("None of the configuration files found, exiting\n");
	  return -1;
	}
    }

  ensure_EEPROM_parameter_integrity ();

  // migration of the EEPROM values into the configuration data file format

  bool result;
  float value;

  // use a temporary data file to fill in all found config parameters
  uint32_t temp_file_storage[1024];
  EEPROM_file_system <LOWEST_UNUSED_EEPROM_ID> temp_file;
  temp_file.initialize_memory_area( temp_file_storage, temp_file_storage+1024);

  value = configuration (SENS_TILT_ROLL);
  result = temp_file.store_data (SENS_TILT_ROLL, 1, &value);

  value = configuration (SENS_TILT_PITCH);
  result = temp_file.store_data (SENS_TILT_PITCH, 1, &value);

  value = configuration (SENS_TILT_YAW);
  result = temp_file.store_data (SENS_TILT_YAW, 1, &value);

  value = configuration (PITOT_OFFSET);
  result = temp_file.store_data (PITOT_OFFSET, 1, &value);

  value = configuration (PITOT_SPAN);
  result = temp_file.store_data (PITOT_SPAN, 1, &value);
  assert(result == true);

  value = configuration (QNH_OFFSET);
  result = temp_file.store_data (QNH_OFFSET, 1, &value);
  assert(result == true);

  value = configuration (VARIO_TC);
  result = temp_file.store_data (VARIO_TC, 1, &value);
  assert(result == true);

  value = configuration (VARIO_INT_TC);
  result = temp_file.store_data (VARIO_INT_TC, 1, &value);
  assert(result == true);

  value = configuration (VARIO_P_TC);
  result = temp_file.store_data (VARIO_P_TC, 1, &value);
  assert(result == true);

  value = configuration (WIND_TC);
  result = temp_file.store_data (WIND_TC, 1, &value);
  assert(result == true);

  value = configuration (MEAN_WIND_TC);
  result = temp_file.store_data (MEAN_WIND_TC, 1, &value);
  assert(result == true);

  value = configuration (HORIZON);
  result = temp_file.store_data (HORIZON, 1, &value);
  assert(result == true);

  value = configuration (GNSS_CONFIGURATION);
  result = temp_file.store_data (GNSS_CONFIGURATION, 1, &value);
  assert(result == true);

  value = configuration (ANT_BASELENGTH);
  result = temp_file.store_data (ANT_BASELENGTH, 1, &value);
  assert(result == true);

  value = configuration (ANT_SLAVE_DOWN);
  result = temp_file.store_data (ANT_SLAVE_DOWN, 1, &value);
  assert(result == true);

  value = configuration (ANT_SLAVE_RIGHT);
  result = temp_file.store_data (ANT_SLAVE_RIGHT, 1, &value);
  assert(result == true);

  temp_file.dump_all_entries ();

  // now we initialize the permanent data file
  permanent_data_file.initialize_memory_area(
	  (uint32_t *)permanent_data_file_storage,
	  (uint32_t *)(permanent_data_file_storage+EEPROM_FILE_SYSTEM_SIZE));

  // and copy all collected information into it
  permanent_data_file.import_all_data( temp_file);

  // do some tests
  assert( permanent_data_file.is_consistent());
  permanent_data_file.dump_all_entries ();

  return true;
}

void write_permanent_data_file( char * path)
{
  assert( permanent_data_file.is_consistent() );
  char file_path[100];
  strcpy( file_path, path);
  char * slash = strrchr( file_path, '/');
  if( slash == 0)
    return;
  slash[1]=0;
  strcat( file_path, "configuration.lrsx");
  ofstream perm_data_file_stream ( file_path, ios::out | ios::binary | ios::ate);
  if (!perm_data_file_stream.is_open ())
    {
      printf ("cannot open file : configuration_data_file.dat - closing");
      exit (0);
    }

  perm_data_file_stream.write ((const char*) permanent_data_file_storage, EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
  perm_data_file_stream.close ();

  printf("permanent setup data:\n");
  permanent_data_file.dump_all_entries ();
}

#define CONFIG_SIZE 1024

void read_permanent_data_file( char * file_path_name)
{
  ifstream in_file ( file_path_name, ios::in | ios::binary | ios::ate);
  if (not in_file.is_open ())
    return;

  uint32_t data[ CONFIG_SIZE];
  in_file.seekg (0, ios::beg);

  if( not in_file.read ( (char*) data, sizeof( data)))
    return;

  EEPROM_file_system <LOWEST_UNUSED_EEPROM_ID> temp_file;
  if( not temp_file.set_memory_to_existing_data( data, data+CONFIG_SIZE))
    return; // inconsistent

  // now we initialize the permanent data file
  permanent_data_file.initialize_memory_area(
	  (uint32_t *)permanent_data_file_storage,
	  (uint32_t *)(permanent_data_file_storage+EEPROM_FILE_SYSTEM_SIZE));

  // and copy all collected information into it
  permanent_data_file.import_all_data( temp_file);
  printf("Extra configuration read:\n");
  permanent_data_file.dump_all_entries ();
}
