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

#define TCP_PORT 8880	// XCsoar wants this one

#define WITH_DENSITY_DATA	1
#define PARALLEL_MAGNETIC_AHRS 	1
#define INCLUDING_NANO	 	1

#define MINIMUM_MAG_CALIBRATION_SAMPLES 6000
#define MAG_CALIB_LETHARGY	0.8f // percentage of remaining old calibration info
#define MAG_CALIBRATION_CHANGE_LIMIT 5.0e-4f // variance average of changes: 3 * { offset, scale }

#define CIRCLE_LIMIT 		(10 * 100) //!< 10 * 1/100 s delay into / out of circling state
#define STABLE_CIRCLING_LIMIT	(30 * 100) // seconds @ 100 Hz for MAG auto calibration

#endif /* SRC_SYSTEM_CONFIGURATION_H_ */
