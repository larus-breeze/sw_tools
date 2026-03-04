/***********************************************************************//**
 * @file    		system_configuration.h
 * @brief   		collection of system tuning parameters
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

#ifndef SRC_SYSTEM_CONFIGURATION_H_
#define SRC_SYSTEM_CONFIGURATION_H_

#define DEBUG

#define GIT_TAG_DEC 0x12345678 // dummy

#define UNIX 1   // Indicates to the lib that running on Unix/Windows but not on stm32

#define TCP_PORT 8880	// XCsoar wants this one

#define INCLUDING_NANO	 		1

#define DEVELOPMENT_ADDITIONS		1
#define EEPROM_WRITES_LOGGED		1
#define DISABLE_SAT_COMPASS		0
#define ENABLE_LINUX_CAN_INTERFACE	1
#define REPORT_MAGNETIC_CALIBRATION	1

#define WITH_EXTERNAL_MAGNETOMETER	0
#define SIMULATE_EXTERNAL_MAGNETOMETER	0

#define PRINT_3D_MAG_PARAMETERS		1

#endif /* SRC_SYSTEM_CONFIGURATION_H_ */
