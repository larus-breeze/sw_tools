#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "UDP_server.h"


static int sockfd;
static sockaddr_in dest_addr{};

bool open_UDP_port(void)
{
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(5005);
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  
    return true;
}

void write_UDP_port( char * data, unsigned length)
{

  ssize_t sent_bytes = sendto(sockfd, data, length, 0,
                              (struct sockaddr*)&dest_addr, sizeof(dest_addr));
  if (sent_bytes < 0) {
      perror("sendto");
  }
      
}


