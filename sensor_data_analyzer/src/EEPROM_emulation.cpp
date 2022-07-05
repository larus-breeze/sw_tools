#include <iostream>
#include <fstream>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
using namespace std;

#include "EEPROM_emulation.h"

config_param_type config_parameters[EEPROM_PARAMETER_ID_END];

float configuration( EEPROM_PARAMETER_ID id)
{
	if( config_parameters[id].identifier == id)
		return config_parameters[id].value;
	else
		return 0.0f;
}

const persistent_data_t * find_parameter_from_ID( EEPROM_PARAMETER_ID id);

bool read_EEPROM_value( EEPROM_PARAMETER_ID id, float &value)
{
	if( config_parameters[id].identifier == id)
	{
		value = config_parameters[id].value;
		return true;
	}

	return false;
}

bool write_EEPROM_value( EEPROM_PARAMETER_ID id, float value)
{
  float old_value = config_parameters[id].value;
  config_parameters[id].value = value;
  cout << "EEPROM(" << id << ")=" << old_value << "->" << value << endl;
  return false;
}

bool EEPROM_initialize( void)
{
  return true;
}

