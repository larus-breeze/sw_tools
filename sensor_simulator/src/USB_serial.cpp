/***********************************************************************//**
 * @file		USB_serial.cpp
 * @brief		Interface to USB -> RS232 (implementation)
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

#ifdef _WIN32
#include "USB_serial.h"
#include <stdint.h>
bool open_USB_serial(char *portname) {
  return false;
}

bool write_usb_serial(uint8_t *data, unsigned size) {
  return false;
}

#else

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include "USB_serial.h"

int fd;

int set_interface_attribs (int fd, int speed, int parity)
{
  struct termios tty;
  if (tcgetattr (fd, &tty) != 0)
      return -1;

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit symbols
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 5;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // no XON XOFF

  tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls + enable reading
  tty.c_cflag &= ~(PARENB | PARODD); // no parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
      return -1;
  else
    return 0;
}

void set_blocking (int fd, int should_block)
{
  struct termios tty_settings;
  memset (&tty_settings, 0, sizeof tty_settings);
  if (tcgetattr (fd, &tty_settings) != 0)
      return;

  tty_settings.c_cc[VMIN] = should_block ? 1 : 0;
  tty_settings.c_cc[VTIME] = 5;    // 5 + 100ms timeout

  tcsetattr (fd, TCSANOW, &tty_settings);
}

bool open_USB_serial ( char *portname)
{
  fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0)
      return false;

  set_interface_attribs (fd, B115200, 0);
  set_blocking (fd, 0);

  return true;
}

bool write_usb_serial( uint8_t * data, unsigned size)
{
  if( fd == 0)
    return 0; // silently give up

  return 0 == write (fd, data, size);
}

#endif
