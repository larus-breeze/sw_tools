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

int sockfd, connfd;
struct sockaddr_in servaddr, cli;
socklen_t len;

bool open_TCP_port(void)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed\n");
        return false;
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if ((bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Bind failed\n");
        return false;
    }

    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed\n");
        return false;
    }
    else
        printf("Server listening\n");

    len = sizeof(cli);
    return true;
}

bool wait_and_accept_TCP_connection( void)
{
  connfd = accept(sockfd, (sockaddr*)&cli, &len);
  if (connfd < 0) {
      printf("server accept failed...\n");
      return false;
  }
  else
      printf("server accept the client...\n");
  return true;
}

void write_TCP_port( char * data, unsigned length)
{
  if( connfd)
    write(connfd, data, length);
}

void close_TCP_port(void)
{
  if( sockfd)
    close(sockfd);
}
