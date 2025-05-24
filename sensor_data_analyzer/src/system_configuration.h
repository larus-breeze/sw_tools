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

#define UNIX 1
#define _WIN32 1

#define TCP_PORT 8880	// XCsoar wants this one

#define DATA_FORMAT_2022	0 // use this to analyze year 2022 and older data files

#if DATA_FORMAT_2022

#define WITH_LOWCOST_SENSORS	1
#define WITH_DENSITY_DATA	1
#define WITH_DENSITY_DUMMY	0
#define VERTICAL_SPEED_INVERTED 0
#define INCLUDING_NANO	 	1
#define USE_LOWCOST_IMU		0
#define NEW_DATA_FORMAT 	0

#else // for MK2 (actual) sensor data

#define WITH_LOWCOST_SENSORS	0
#define WITH_DENSITY_DATA	0
#define INCLUDING_NANO	 	1
#define USE_LOWCOST_IMU		0
#define NEW_DATA_FORMAT 	1

#endif

#define DEVELOPMENT_ADDITIONS		1
#define MAGNETIC_DECISION_OVERRIDE 	0
#define EEPROM_WRITES_LOGGED		1
#define DISABLE_SAT_COMPASS		0
#define USE_LARUS_NMEA_EXTENSIONS	1
#define ENABLE_LINUX_CAN_INTERFACE	0

#endif /* SRC_SYSTEM_CONFIGURATION_H_ */
