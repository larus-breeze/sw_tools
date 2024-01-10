/***********************************************************************//**
 * @file		CAN_socket_driver.h
 * @brief		I/O over the generic Linux CANsocket interface
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

#ifndef CAN_SOCKET_DRIVER_H_
#define CAN_SOCKET_DRIVER_H_

#ifndef _WIN32

#include "generic_CAN_driver.h"
#include <CAN_socket_driver.h>

bool  CAN_socket_initialize(void);
bool CAN_socket_close( void);
int  CAN_socket_send(const CANpacket &p);
bool CAN_socket_is_open( void);

#endif // _WIN32
#endif /* CAN_SOCKET_DRIVER_H_ */
