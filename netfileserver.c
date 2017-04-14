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

#define MIN_BUFF_SIZE 5

void error(const char *msg) {
    fprintf(stderr,"%s L:%d\n",msg,__LINE__);
    exit(1);
}

char * tryNum(char * ascii, int max_size) {
  while(*ascii != ','){
    if(max_size == 0)
      return NULL;
    ascii++;
    max_size--;
  }

  return ascii;
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
  char buffer[250]; //Buffer for reading input and writing output
  //Zero out buffer and read message into buffer
  bzero(buffer,250);

  //Create buffer to read Packet Size
  char * init_buffer = malloc(1);
  if(init_buffer == NULL)
    error("ERROR with malloc");

  //Create and Initialize reading vars
  int status, amount, total_Size = 0;
  int offset = 0;
  char * numEnd = NULL;

  while(numEnd == NULL){ //While we still haven't found a comma
    total_Size += MIN_BUFF_SIZE;

    //Realloc to ensure you have enough room
    init_buffer = realloc(init_buffer, total_Size);
    if(init_buffer == NULL)
      error("ERROR with realloc");


    //Read bytes into init_buffer
    amount = MIN_BUFF_SIZE;
    status = 0;
    while(amount > 0) { //Ensure MIN_BUFF_SIZE bytes were read...
      status = read (*((int*) sock), init_buffer+offset, amount);
      if (status < 0)
        error("ERROR reading from socket");
      amount -= status;
      offset += status;
    }

    //See if we found the end of the number yet
    numEnd = tryNum(init_buffer, total_Size);
  }

  //Read size into msgSize
  *numEnd = '\0';
  int msgSize = atoi(init_buffer);
  *numEnd = ',';

  printf("Size: %d\n", msgSize);

  //Malloc for the rest of the message (includng the comma)
  char * msg = malloc(msgSize*sizeof(char));
  if(msg == NULL)
    error("ERROR with malloc");

  int excess_len = total_Size - (numEnd - init_buffer);

  //Copy extra data already read
  msg = memcpy(msg, numEnd, excess_len);

  printf("Msg before: %s\n", msg);

  //Read rest of message into malloced msg char*
  amount = msgSize - excess_len;
  offset = excess_len;
  status = 0;
  while(amount > 0) {
    offset += status;
    status = read (*((int*) sock), msg+offset, amount);
    if (status >= 0) {
      amount -= status;
    } else {
      error("ERROR reading from socket");
    }
  }

  printf("Msg: %s\n", msg);

  //Free the buffer now that we're done with it
  free(init_buffer);

  char * ptr = msg;


  // _____READ META_DATA_____
  int fd = 0;
  char op = '\0';
  //char * data;

  //read and decide op
  ptr++;
  op = *ptr;
  ptr += 2;

  switch(op){
    case 'o': //Open
      printf("Op: open() ");
      //getMalcMsg()
      break;

    case 'r': //Read
      printf("Op: read() ");
      numEnd = tryNum(ptr, msgSize - 3); //read fd
      if(numEnd == NULL);
      *numEnd = '\0';
      fd = atoi(ptr);
      *numEnd = ',';
      //getMalcMsg()
      break;

    case 'w': //Write
      printf("Op: write() ");
      numEnd = tryNum(ptr, msgSize - 3); //read fd
      if(numEnd == NULL);
      *numEnd = '\0';
      fd = atoi(ptr);
      *numEnd = ',';
      //getMalcMsg()
      break;

    case 'c': //Close
      printf("Op: close() ");
      numEnd = tryNum(ptr, msgSize - 3); //read fd
      if(numEnd == NULL);
      *numEnd = '\0';
      fd = atoi(ptr);
      *numEnd = ',';
      //getMalcMsg()
      break;

    default:
      error("ERROR Not an OP");
  }

  printf("Fd: %d\n", fd);

  //Free msg after use
  free(msg);

  //Take a message into buffer from stdin
  printf("\nPlease enter returning message: ");
  bzero(buffer,250);
  fgets(buffer,250,stdin);

  //Write the message back to the client
  status = write(*((int*) sock),buffer,250);
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
