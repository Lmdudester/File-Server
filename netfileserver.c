/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno; //Info required for server
     socklen_t clilen; //WHAT IS THIS?

     char buffer[256]; //Buffer for reading input and writing output

     struct sockaddr_in serv_addr, cli_addr; //For setting up socket
     int n; //wot?

     //Throw an error if a port isn't given to ./server
     if (argc != 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     //Create a socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");

     portno = atoi(argv[1]); //Store port number

     //Build sockaddr_in struct (4? steps)
     bzero((char *) &serv_addr, sizeof(serv_addr)); //1)Zero out space

     serv_addr.sin_family = AF_INET; //2) Set addr family to AF_INET

     serv_addr.sin_addr.s_addr = INADDR_ANY; //Set to accept any IP

     serv_addr.sin_port = htons(portno); //3) Set up the port

     //???
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
          error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     while(1 == 1){
          //Wait for client
          newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                             &clilen);
          if (newsockfd < 0)
               error("ERROR on accept");

          //Zero out buffer and read message into buffer
          bzero(buffer,256);
          n = read(newsockfd,buffer,255);
          if (n < 0) error("ERROR reading from socket");
          printf("Here is the message: %s\n",buffer);

          //Take a message into buffer from stdin
          printf("Please enter returning message: ");
          bzero(buffer,256);
          fgets(buffer,255,stdin);

          //Write the message back to the client
          n = write(newsockfd,buffer,255);
          if (n < 0)
               error("ERROR writing to socket");
     }
     //Close Socket
     close(newsockfd);
     close(sockfd);

     return 0;
}
