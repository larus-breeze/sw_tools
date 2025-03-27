/***********************************************************************//**
 * @file		EEPROM_emulation.h
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

#ifndef EEPROM_EMULATION_H_
#define EEPROM_EMULATION_H_

#include "persistent_data.h"
float configuration( EEPROM_PARAMETER_ID id);

//! this structure describes one persistent parameter
struct config_param_type
{
	unsigned identifier;
	float value;
};

extern config_param_type config_parameters[EEPROM_PARAMETER_ID_END];

int read_EEPROM_file (char *basename);
bool write_EEPROM_dump( char * basename);

#endif /* EEPROM_EMULATION_H_ */
