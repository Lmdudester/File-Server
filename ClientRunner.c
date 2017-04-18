#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORTNUM 10897

static char * verifiedHost = NULL;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int getSock(char * hostname){
  int sockfd, portno; //Info required for client
  struct sockaddr_in serv_addr; //Info for setting up socket
  struct hostent *server; //Info for setting up socket

  //Get port number
  portno = PORTNUM;

  //Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
      error("ERROR opening socket");

  //Connect to specific server
  server = gethostbyname(hostname);
  if (server == NULL) {
      return -1;
  }

  //Build sockaddr_in struct (4 steps)
  bzero((char *) &serv_addr, sizeof(serv_addr)); //1) Zero out space

  serv_addr.sin_family = AF_INET; //2) Set addr family to AF_INET

  serv_addr.sin_port = htons(portno); //3) Set up the port

  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length); //4) Copy representation of IP from hostent struct to sockaddr_in


  //Try to connect, if connect fails, ERRNO is set
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
      error("ERROR connecting");

  return sockfd;
}

int netserverinit(char * hostname){
  int status, sockfd;
  char buffer[5]; //For what to send to server

  sockfd = getSock(hostname);
  if(sockfd < 0){
    error("ERROR verifying host");
  }

  //---- Successfully Connected ----//
  bzero(buffer,5);
  strcpy(buffer, "chk?");

  //Write this buffer to the socket
  status = write(sockfd,buffer,5);
  if (status < 0)
       error("ERROR writing to socket");

  //Zero out the buffer for a read
  bzero(buffer,5);

  //Read from the socket into the buffer
  status = read(sockfd,buffer,5);
  if (status < 0)
       error("ERROR reading from socket");

  //Close the socket using sockfd
  close(sockfd);

  //Print read information
  if(strcmp(buffer,"good") == 0){
    verifiedHost = malloc((strlen(hostname)*sizeof(char)) + 1);
    if(verifiedHost == NULL){
      error("ERROR malloc failure");
    }
    *verifiedHost = '\0';
    strcpy(verifiedHost, hostname);
    return 0;
  }

  return -1;
}

int main(int argc, char *argv[])
{
    if(netserverinit(argv[1]) == 0){
      printf("Success\n%s\n", verifiedHost);
    }

    return 0;
}
