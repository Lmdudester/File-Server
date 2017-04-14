#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n; //Info required for client
    struct sockaddr_in serv_addr; //Info for setting up socket
    struct hostent *server; //Info for setting up socket

    char buffer[250]; //For what to send to server

    if (argc != 3) { //Prints usage
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(1);
    }

    //Get port number
    portno = atoi(argv[2]);

    //Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    //Connect to specific server
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
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

    //---- Successfully Connected ----//

    //Take a message into buffer from stdin
    printf("Please enter the message: ");
    bzero(buffer,250);
    fgets(buffer,250,stdin);

    //Write this buffer to the socket
    n = write(sockfd,buffer,250);
    if (n < 0)
         error("ERROR writing to socket");

    //Zero out the buffer for a read
    bzero(buffer,250);

    //Read from the socket into the buffer
    n = read(sockfd,buffer,250);
    if (n < 0)
         error("ERROR reading from socket");

    //Print read information
    printf("Returned Message: %s\n",buffer);

    //Close the socket using sockfd
    close(sockfd);
    return 0;
}
