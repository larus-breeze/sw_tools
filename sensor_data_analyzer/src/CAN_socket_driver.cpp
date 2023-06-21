/***********************************************************************//**
 * @file		CAN_socket_driver.cpp
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

#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <CAN_socket_driver.h>

int CAN_socket;

int CAN_socket_initialize(void)
{
  struct sockaddr_can addr;
  struct ifreq ifr;

  if ((CAN_socket = socket (PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
      perror ("CAN Socket");
      return 1;
    }

  strcpy (ifr.ifr_name, "can0");
  ioctl (CAN_socket, SIOCGIFINDEX, &ifr);

  memset (&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind (CAN_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
      perror ("CAN Bind");
      return 1;
    }
}

bool CAN_socket_is_open( void)
{
  return CAN_socket != 0;
}

bool CAN_socket_close( void)
{
  if( CAN_socket == 0)
    return false;
  return (close (CAN_socket) < 0);
}

int CAN_socket_send(const CANpacket &p)
{
  if( CAN_socket == 0)
    return 0;

  struct can_frame frame;
  frame.can_id = p.id;
  frame.can_dlc = p.dlc;
  *(uint64_t *)(frame.data)=p.data_l;

  int retv = write (CAN_socket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame);
  if( retv < 0)
    {
      close (CAN_socket);
      CAN_socket = 0;
    }
  return retv;
}

#endif // _WIN32
