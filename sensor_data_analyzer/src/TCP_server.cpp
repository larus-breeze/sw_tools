#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "TCP_server.h"

#define MAX 80
#define PORT 8880

#define SA struct sockaddr
int sockfd, connfd;
struct sockaddr_in servaddr, cli;
socklen_t len;

bool open_TCP_port(void)
{
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        return false;
    }
//    else
//        printf("Socket successfully created..\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        return false;
    }
//    else
//        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        return false;
    }
    else
        printf("Server listening..\n");

    len = sizeof(cli);
    return true;
}

bool wait_and_accept_TCP_connection( void)
{
  // Accept the data packet from client
  connfd = accept(sockfd, (SA*)&cli, &len);
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
