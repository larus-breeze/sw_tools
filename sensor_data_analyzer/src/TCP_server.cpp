/***********************************************************************//**
 * @file		TCP_server.cpp
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

#include "system_configuration.h"
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "TCP_server.h"

int listening_socket_file_descriptor;
struct sockaddr_in servaddr, cli;
socklen_t len;

enum {MAX_CLIENTS=10};
int TCP_clients[MAX_CLIENTS];
int number_of_TCP_clients = 0;

bool accept_TCP_client( bool wait_for_client)
{
  if( number_of_TCP_clients >= MAX_CLIENTS)
    return false;

  int client_descriptor;
  while( true)
    {
      client_descriptor = accept(listening_socket_file_descriptor, (sockaddr*)&cli, &len);
      if( client_descriptor > 0)
	{
        printf("New client accepted\n");
        break;
	}
      if( ! wait_for_client)
	  return false;
      sleep( 1);
    }

  TCP_clients[number_of_TCP_clients]=client_descriptor;
  ++number_of_TCP_clients;
  return true;
}

bool open_TCP_port(void)
{
  listening_socket_file_descriptor = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listening_socket_file_descriptor == -1) {
        printf("Socket creation failed\n");
        return false;
    }

    int opt = 0;
    if( setsockopt( listening_socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        printf("Socket modification failed\n");
        return false;
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if ((bind(listening_socket_file_descriptor, (sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Bind failed\n");
        return false;
    }

    if ((listen(listening_socket_file_descriptor, 2)) != 0) {
        printf("Listen failed\n");
        return false;
    }
    else
        printf("Server listening\n");

    len = sizeof(cli);
    return true;
}

void write_TCP_port( char * data, unsigned length)
{
  accept_TCP_client(false);

  int written_bytes;
  unsigned remaining_length;
  char * data_tail;

  for( int client_index = 0; client_index < number_of_TCP_clients; ++client_index)
    {
      data_tail = data;
      remaining_length = length;
      do
	{
	  written_bytes = write(TCP_clients[client_index], data_tail, remaining_length);
	  if( written_bytes < 0)
	    continue; // we have got some error and give up
	  remaining_length -= written_bytes;
	  data_tail += written_bytes;
	}
      while( remaining_length > 0);
    }
}

void close_TCP_port(void)
{
  if( listening_socket_file_descriptor)
    close(listening_socket_file_descriptor);

  for( int client_index = 0; client_index < number_of_TCP_clients; ++client_index)
    close( TCP_clients[client_index]);
}
