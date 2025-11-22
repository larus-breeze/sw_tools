/***********************************************************************//**
 * @file		CAN_USB_gateway.cpp
 * @brief		CAN-bus packets tunneled via RS232
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

#include "USB_serial.h"
#include "generic_CAN_driver.h"
#include <CAN_socket_driver.h>
#include "UDP_server.h"

bool CAN_send( const CANpacket &p, unsigned)
{

  uint8_t length = 2 + p.dlc;
  uint16_t message[5];
  message[0] = p.id;
  for (int i = 1; i<5; i++){
    message[i] = p.data_h[i-1];
  }

  //Output CAN Date via UDP in order to use the display simulation tool
  write_UDP_port((char *)message, length);
  CAN_gateway_packet output( p);

  write_usb_serial( (uint8_t *) &output, sizeof output);

#ifndef _WIN32
  if( CAN_socket_is_open())
    CAN_socket_send(p);
#endif
  return true;




}



