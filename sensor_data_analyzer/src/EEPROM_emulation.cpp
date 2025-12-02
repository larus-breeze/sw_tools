/***********************************************************************//**
 * @file		EEPROM_emulation.cpp
 * @brief		Replacement on the PC for the nonvolatile memory of the micro-controller
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

#include <iostream>
#include <fstream>
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "system_configuration.h"
#include "ascii_support.h"

using namespace std;

#include "EEPROM_emulation.h"

#include <istream>
#include <string>

config_param_type config_parameters[EEPROM_PARAMETER_ID_END];

float configuration( EEPROM_PARAMETER_ID id)
{
	if( id < EEPROM_PARAMETER_ID_END && config_parameters[id].identifier == id)
		return config_parameters[id].value;
	else
		return 0.0f;
}

const persistent_data_t * find_parameter_from_ID( EEPROM_PARAMETER_ID id);

bool read_EEPROM_value( EEPROM_PARAMETER_ID id, float &value)
{
	if( id < EEPROM_PARAMETER_ID_END && config_parameters[id].identifier == id)
	{
		value = config_parameters[id].value;
		return false;
	}

	return true;
}

bool write_EEPROM_value( EEPROM_PARAMETER_ID id, float value)
{
#if EEPROM_WRITES_LOGGED
  assert( id < EEPROM_PARAMETER_ID_END);
  float old_value = config_parameters[id].value;
  config_parameters[id].value = value;
  config_parameters[id].identifier = id;
  cout << "EEPROM(" << id << ")=" << old_value << "->" << value << endl;
#endif
  return false;
}

bool EEPROM_initialize( void)
{
  return true;
}

int
read_identifier (const char *s)
{
  if (s[2] != ' ')
    return EEPROM_PARAMETER_ID_END;
  int identifier = atoi (s);
  if ((identifier > 0) && (identifier < EEPROM_PARAMETER_ID_END))
    return identifier;
  return EEPROM_PARAMETER_ID_END; // error
}
bool lock_EEPROM(bool)
{
  return true; // just a stub
}

int read_EEPROM_file (char *basename)
{
  char buf[200];
  strcpy (buf, basename);
  strcat (buf, ".EEPROM");

#ifdef _WIN32  // _AUGUST
  const char *line = NULL;
  size_t len = 0;
  ifstream inFile;
  string sline;

  inFile.open(buf);

  for (getline(inFile, sline);
       sline.length();
       getline(inFile, sline)) {
    line = sline.c_str();
#else
  FILE *fp = fopen(buf, "r");
  if (fp == NULL)
    return (EXIT_FAILURE);

  char *line = NULL;
  size_t len = 0;
  const persistent_data_t *param;

  while ((getline(&line, &len, fp)) != -1)
    {
#endif
    bool new_format = false;
    EEPROM_PARAMETER_ID identifier = (EEPROM_PARAMETER_ID)read_identifier(line);
    if (identifier == EEPROM_PARAMETER_ID_END)
      {
	param = find_parameter_from_name( line);
	new_format = true;
      }
    else
	param = find_parameter_from_ID(identifier);

    if( param == 0)
      continue;

    identifier = param->id;

    unsigned name_len = strlen(param->mnemonic);

    if(
	( ! new_format)
	&&
	(0 != strncmp( (const char *)(param->mnemonic), (const char *)line + 3, name_len))
      )
      continue;

      float value;

      if ( new_format)
	{
	if( line[name_len + 1] != '=')
	    continue;
	value = atof(line + name_len + 3 );
	}
      else
	{
	  if( line[name_len + 4] != '=')
	    continue;
	  value = atof(line + name_len + 6 );
	}


    if( param->is_an_angle)
      value *= (M_PI / 180.0);

    config_parameters[identifier].identifier = identifier;
    config_parameters[identifier].value = value;

#ifdef _WIN32 // _AUGUST
  }
  inFile.close();
#else 
  }
  fclose (fp);
  if (line)
    free (line);
#endif

  return 0;
}

bool write_EEPROM_dump( char * basename)
{
  char buffer[200];
  char *next = buffer;

  strcpy(buffer, basename);
  strcat( buffer, "/config.EEPROM");
  ofstream outfile ( buffer, ios::out | ios::binary | ios::ate);
  if ( ! outfile.is_open ())
    return true;

  for( unsigned index = 0; index < PERSISTENT_DATA_ENTRIES; ++index)
    {
      float value;
      bool result = read_EEPROM_value( PERSISTENT_DATA[index].id, value);
      if( ! result)
	{
	  if( PERSISTENT_DATA[index].is_an_angle)
	    value *= 180.0 / M_PI; // format it human readable

	  next = buffer;
	  append_string( next, PERSISTENT_DATA[index].mnemonic);
	  append_string (next," = ");
	  next = my_ftoa (next, value);
	  *next++='\r';
	  *next++='\n';
	  *next=0;

	  outfile.write ( (const char *)buffer, next-buffer);
	}
      }

  outfile.close ();
  return false;
}




