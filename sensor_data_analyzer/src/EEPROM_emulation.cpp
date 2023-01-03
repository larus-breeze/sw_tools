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
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
using namespace std;

#include "EEPROM_emulation.h"

#include <istream>
#include <string>

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
		return false;
	}

	return true;
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
  while ((getline(&line, &len, fp)) != -1) {
#endif
    EEPROM_PARAMETER_ID identifier =
        (EEPROM_PARAMETER_ID)read_identifier(line);
    if (identifier == EEPROM_PARAMETER_ID_END)
      continue;
    const persistent_data_t *param = find_parameter_from_ID(identifier);
    unsigned name_len = strlen(param->mnemonic);
    if (0 != strncmp((const char *)(param->mnemonic), (const char *)(line + 3),
                     name_len))
      continue;
    if (line[name_len + 4] != '=')
      continue;
    float value = atof(line + name_len + 6);

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


