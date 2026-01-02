#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "persistent_data_file.h"
#include "persistent_data_file_emulation.h"
#include "EEPROM_emulation.h"
#include "data_structures.h"
#include "persistent_data.h"
#include "compass_calibrator_3D.h"

using namespace std;

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

  strcat (path, "configuration_data_file.dat");

  // FIRST preference: read the configuration data file
  ifstream config_file (path, ios::in | ios::binary | ios::ate);
  if (config_file.is_open ())
    {
      streampos size = config_file.tellg ();
      if (size != (EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t)))
	{
	  printf ("configuration_data_file.dat : wrong file size\n");
	  return -1;
	}

      config_file.seekg (0, ios::beg);
      config_file.read ((char*) permanent_file, size);
      config_file.close ();

      bool result = permanent_data_file.setup (permanent_file, EEPROM_FILE_SYSTEM_SIZE);
      assert(result == true);

      assert( permanent_data_file.is_consistent ());
      printf ("\nConfig file read:\n");
      permanent_data_file.dump_all_entries ();

      float *compass_calibrator_3d_data = new float[ 12];
      assert( compass_calibrator_3d_data);

      bool using_orientation_defaults = false;
      bool parameters_available = permanent_data_file.retrieve_data( MAG_SENSOR_XFER_MATRIX, 12, compass_calibrator_3d_data);
      if( not parameters_available) // use sensor orientation data as a first approximation for the sensor transfer matrix
	{
	  float roll, pitch, yaw;
	  permanent_data_file.retrieve_data( SENS_TILT_ROLL,  1, &roll);
	  permanent_data_file.retrieve_data( SENS_TILT_PITCH, 1, &pitch);
	  permanent_data_file.retrieve_data( SENS_TILT_YAW,   1, &yaw);

	  float3matrix rotation_matrix;
	  quaternion<float> q;
	  q.from_euler ( roll, pitch, yaw);
	  q.get_rotation_matrix ( rotation_matrix);

	  compass_calibrator_3d_data[0] = ZERO;
	  compass_calibrator_3d_data[1] = rotation_matrix.e[0][0];
	  compass_calibrator_3d_data[2] = rotation_matrix.e[0][1];
	  compass_calibrator_3d_data[3] = rotation_matrix.e[0][2];

	  compass_calibrator_3d_data[4] = ZERO;
	  compass_calibrator_3d_data[5] = rotation_matrix.e[1][0];
	  compass_calibrator_3d_data[6] = rotation_matrix.e[1][1];
	  compass_calibrator_3d_data[7] = rotation_matrix.e[1][2];

	  compass_calibrator_3d_data[8] = ZERO;
	  compass_calibrator_3d_data[9]  = rotation_matrix.e[2][0];
	  compass_calibrator_3d_data[10] = rotation_matrix.e[2][1];
	  compass_calibrator_3d_data[11] = rotation_matrix.e[2][2];
	  using_orientation_defaults = true;
	}
      compass_calibrator_3D.set_current_parameters( compass_calibrator_3d_data, using_orientation_defaults);

      parameters_available = permanent_data_file.retrieve_data( EXT_MAG_SENSOR_XFER_MATRIX, 12, compass_calibrator_3d_data);
      if( parameters_available)
	external_compass_calibrator_3D.set_current_parameters( compass_calibrator_3d_data);

      delete[] compass_calibrator_3d_data;

#if 0 // test data import
      {
	EEPROM_file_system_node copy_data_storage[EEPROM_FILE_SYSTEM_SIZE];
	EEPROM_file_system permanent_data_copy( copy_data_storage, copy_data_storage+EEPROM_FILE_SYSTEM_SIZE);
	bool result = permanent_data_copy.setup ( copy_data_storage, EEPROM_FILE_SYSTEM_SIZE);
	assert(result == true);
	permanent_data_copy.import_all_data( permanent_data_file);
	permanent_data_copy.dump_all_entries ();
      }
#endif

      using_permanent_data_file = true;
    }
  else
    {
      // SECOND preference: Read the meta-data in "config.EEPROM"
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
      memset( permanent_file, 0xff, EEPROM_FILE_SYSTEM_SIZE * sizeof( uint32_t));
      bool result = permanent_data_file.setup (permanent_file, EEPROM_FILE_SYSTEM_SIZE);
      assert(result == true);

      float value;

      value = configuration (SENS_TILT_ROLL);
      result = permanent_data_file.store_data (SENS_TILT_ROLL, 1, &value);

      value = configuration (SENS_TILT_PITCH);
      result = permanent_data_file.store_data (SENS_TILT_PITCH, 1, &value);

      value = configuration (SENS_TILT_YAW);
      result = permanent_data_file.store_data (SENS_TILT_YAW, 1, &value);

      value = configuration (PITOT_OFFSET);
      result = permanent_data_file.store_data (PITOT_OFFSET, 1, &value);

      value = configuration (PITOT_SPAN);
      result = permanent_data_file.store_data (PITOT_SPAN, 1, &value);
      assert(result == true);

      value = configuration (QNH_OFFSET);
      result = permanent_data_file.store_data (QNH_OFFSET, 1, &value);
      assert(result == true);

      value = configuration (VARIO_TC);
      result = permanent_data_file.store_data (VARIO_TC, 1, &value);
      assert(result == true);

      value = configuration (VARIO_INT_TC);
      result = permanent_data_file.store_data (VARIO_INT_TC, 1, &value);
      assert(result == true);

      value = configuration (VARIO_P_TC);
      result = permanent_data_file.store_data (VARIO_P_TC, 1, &value);
      assert(result == true);

      value = configuration (WIND_TC);
      result = permanent_data_file.store_data (WIND_TC, 1, &value);
      assert(result == true);

      value = configuration (MEAN_WIND_TC);
      result = permanent_data_file.store_data (MEAN_WIND_TC, 1, &value);
      assert(result == true);

      value = configuration (HORIZON);
      result = permanent_data_file.store_data (HORIZON, round (value));
      assert(result == true);

      value = configuration (GNSS_CONFIGURATION);
      result = permanent_data_file.store_data (GNSS_CONFIGURATION,
					       round (value));
      assert(result == true);

      value = configuration (ANT_BASELENGTH);
      result = permanent_data_file.store_data (ANT_BASELENGTH, 1, &value);
      assert(result == true);

      value = configuration (ANT_SLAVE_DOWN);
      result = permanent_data_file.store_data (ANT_SLAVE_DOWN, 1, &value);
      assert(result == true);

      value = configuration (ANT_SLAVE_RIGHT);
      result = permanent_data_file.store_data (ANT_SLAVE_RIGHT, 1, &value);
      assert(result == true);

      permanent_data_file.dump_all_entries ();
      assert( permanent_data_file.is_consistent());

      char * slash = strrchr( path, '/');
      if( slash != 0)
	slash[1] = 0;
      strcat( path, "configuration_data_file.dat");
      ofstream perm_data_file (path, ios::out | ios::binary | ios::ate);
      if (!perm_data_file.is_open ())
	return false;

      perm_data_file.write ((const char*) permanent_file,
      EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
      perm_data_file.close ();
      printf ("configuration_data_file.dat written - closing");
      exit (0);
    }
  return true;
}

void write_permanent_data_file( char * file_name)
{
  assert( permanent_data_file.is_consistent() );

  EEPROM_file_system_node copy_data_storage[EEPROM_FILE_SYSTEM_SIZE];
  memset( copy_data_storage, 0xff, EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
  EEPROM_file_system permanent_data_copy( copy_data_storage, copy_data_storage+EEPROM_FILE_SYSTEM_SIZE);
  bool result = permanent_data_copy.setup ( copy_data_storage, EEPROM_FILE_SYSTEM_SIZE);
  assert(result == true);
  permanent_data_copy.import_all_data( permanent_data_file);

  char path[100];
  strcpy( path, file_name);
  char * slash = strrchr( path, '/');
  assert( slash != 0);
  slash[1]=0;
  strcat( path, "configuration_data_file.dat");
  ofstream perm_data_file_stream (path, ios::out | ios::binary | ios::ate);
  if (!perm_data_file_stream.is_open ())
    {
      printf ("cannot open file : configuration_data_file.dat - closing");
      exit (0);
    }

  perm_data_file_stream.write ((const char*) copy_data_storage, EEPROM_FILE_SYSTEM_SIZE * sizeof(uint32_t));
  perm_data_file_stream.close ();
}
