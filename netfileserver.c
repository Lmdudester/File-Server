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
#include <ctype.h>

#define MIN_BUFF_SIZE 250

void error(const char *msg) {
    fprintf(stderr,"%s L:%d\n",msg,__LINE__);
    exit(1);
}

int getNum(char * buffer, char ** begin, char ** end){
  int size = 0;
  while(*(*end) != ',') {
    if(*end == buffer + MIN_BUFF_SIZE - 3)
      error("ERROR Size too Large");
    (*end)++;
  }
  *(*end) = '\0';
  size = atoi(*begin);
  *(*end) = ',';
  return size;
}

char * getMalcMsg(){
  //Take whatever it needs to return the associated data malloced
  return NULL;
}

//size, op, <permissions>, fd, data

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
  char buffer[MIN_BUFF_SIZE]; //Buffer for reading input and writing output
  int status = 0; //For testing read-in length, etc...

  //Zero out buffer and read message into buffer
  bzero(buffer,MIN_BUFF_SIZE);

  //Ensure Minimum is read in (Minimum Packet size == 250)
  int amount = MIN_BUFF_SIZE;
  int offset = status;
  while(amount > 0) {
    offset += status;
    status = read (*((int*) sock), buffer+offset, amount);
    if (status >= 0) {
      amount -= status;
    } else {
      error("ERROR reading from socket");
    }
  }

  /* _____READ META_DATA_____ */
  int size = 0, fd = 0;
  char op = '\0';
  //char * data;

  //read size
  char * begin = buffer;
  char * end = buffer;
  size = getNum(buffer, &begin, &end);

  //read and decide op
  end++;
  op = *end;
  end += 2;
  begin = end;

  switch(op){
    case 'o': //Open
      printf("Op: open() ");
      //getMalcMsg()
      break;

    case 'r': //Read
      printf("Op: read() ");
      fd = getNum(buffer, &begin, &end); //read fd
      //getMalcMsg()
      break;

    case 'w': //Write
      printf("Op: write() ");
      fd = getNum(buffer, &begin, &end); //read fd
      //getMalcMsg()
      break;

    case 'c': //Close
      printf("Op: close() ");
      fd = getNum(buffer, &begin, &end); //read fd
      //getMalcMsg()
      break;

    default:
      error("ERROR Not an OP");
  }

  //malloc and send rest to proper op function


  //print Message from client
  printf("Size: %d, Fd: %d Msg: %s\n",size, fd, end);



  //Take a message into buffer from stdin
  printf("\nPlease enter returning message: ");
  bzero(buffer,MIN_BUFF_SIZE);
  fgets(buffer,MIN_BUFF_SIZE,stdin);

  //Write the message back to the client
  status = write(*((int*) sock),buffer,MIN_BUFF_SIZE);
  if (status < 0)
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
     listen(baseSock,5); //Preps to accept clients (only holds 5 clients in queue)
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
