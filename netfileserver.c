/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) {
    fprintf(stderr,"%s L:%d\n",msg,__LINE__);
    exit(1);
}

/*
 * __clientHandler()__
 *  - Handles all the correspondence with a client for a single instance
 *
 *  Arguments:
 *    sock - the socket file descriptor needed to communicate with the client
 *
 *  Returns:
 *    N/A
 */
void * clientHandler(void * sock){
  char buffer[256]; //Buffer for reading input and writing output
  int n; //For testing read-in length, etc...

  //Zero out buffer and read message into buffer
  bzero(buffer,256);
  n = read(*((int*) sock),buffer,255);
  buffer[255] = '\0'; //SET LAST BYTE TO NULL TO END STRING
  if (n < 0) error("ERROR reading from socket");

  //print Message from client
  printf("\nClient Message: %s",buffer);

  //Take a message into buffer from stdin
  printf("\nPlease enter returning message: ");
  bzero(buffer,256);
  fgets(buffer,255,stdin);

  //Write the message back to the client
  n = write(*((int*) sock),buffer,255);
  if (n < 0)
       error("ERROR writing to socket");

  close(*((int*)sock));
  free(sock);
  pthread_exit(0);
}

int main(int argc, char *argv[]) {
     int baseSock, passSock, portNum; //Info required for server
     socklen_t clilen; //Client length -> needed to accept()

     void* (*cliHand)(void*) = clientHandler; //Threading method

     struct sockaddr_in serv_addr, cli_addr; //For setting up sockets

     //Throw an error if a port isn't given to ./server
     if (argc != 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     //Create a socket
     baseSock = socket(AF_INET, SOCK_STREAM, 0);
     if (baseSock < 0)
        error("ERROR opening socket");

     portNum = atoi(argv[1]); //Store port number

     //Build sockaddr_in struct (3 steps)
     bzero((char *) &serv_addr, sizeof(serv_addr)); //1)Zero out space
     serv_addr.sin_family = AF_INET; //2) Set addr family to AF_INET
     serv_addr.sin_port = htons(portNum); //3) Set up the port

     serv_addr.sin_addr.s_addr = INADDR_ANY; //Set to accept any IP

     //Sets up socket
     if (bind(baseSock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
          error("ERROR on binding");
     listen(baseSock,5); //Preps to accept clients
     clilen = sizeof(cli_addr);

     while(1 == 1){
          //Wait for client
          passSock = accept(baseSock, (struct sockaddr *) &cli_addr, &clilen);
          if (passSock < 0)
               error("ERROR on accept");

          //Malloc to pass socket descriptor to cliHand()
          int * pass = malloc(sizeof(int));
          if (pass == NULL)
               error("ERROR on malloc");
          *pass = passSock; //Put it in pass

          //Begin thread creation
          pthread_t cliThread;
          pthread_attr_t cliAttr;

          //Initialize and set Attributes to deatached state to allow exit to end thread
          if(pthread_attr_init(&cliAttr) != 0)
            error("ERROR on attr_init");
          if(pthread_attr_setdetachstate(&cliAttr, PTHREAD_CREATE_DETACHED) != 0)
            error("ERROR on attr_setdetachstate");

          //Create thread and continute
          if(pthread_create(&cliThread, &cliAttr, cliHand, (void*) pass) != 0)
            error("ERROR on create");
     }

     //Close Socket
     //close(passSock);
     close(baseSock);

     return 0;
}
