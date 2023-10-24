/***********************************************************************//**
 * @file		TCP_server.h
 * @brief		TCP server to feed XCsoar with flight data
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

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

bool open_TCP_port(void);
bool accept_TCP_client( bool wait_for_client);
void write_TCP_port( char * data, unsigned length);
unsigned poll_and_read_TCP_port( char * data, unsigned max_length);
void close_TCP_port(void);

#endif /* TCP_SERVER_H_ */
