#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>

#define MIN_BUFF_SIZE 5

/* __threadError()__
 *  - Reports an error and exits, terminates whole program
 *
 *  Arguments:
 *    msg - the error message
 *    line - the line on which the error occured
 *    errNum - the errno value to be set
 *
 *  Returns:
 *    - N/A
 */
void regError(const char * msg, int line, int errNum) {
    fprintf(stderr,"%s ERRNO: \"%s\" L:%d\n",msg, strerror(errNum), line);
    exit(1);
}

/* __threadError()__
 *  - Reports an error without exiting, thread must close itself
 *
 *  Arguments:
 *    msg - the error message
 *    line - the line on which the error occured
 *    errNum - the errno value to be set
 *
 *  Returns:
 *    - N/A
 */
void threadError(const char * msg, int line, int errNum) {
    fprintf(stderr,"%s ERRNO: \"%s\" L:%d\n",msg, strerror(errNum), line);
}

/* __tryNum()__
 *  - Attempts to located a ',' at the end of an ascii number. Will only read
 *      up to max_size. Returns a pointer to the ',' at the end
 *
 *  Arguments:
 *    ascii - the pointer to the beginning of the ascii number
 *    max_size - the maximum chars to be read
 *
 *  Returns:
 *    - a pointer to the ',' at the end if one exists
 *    - NULL otherwise
 */
char * tryNum(char * ascii, int max_size) {
  while(*ascii != ','){
    if(max_size == 0)
      return NULL;
    ascii++;
    max_size--;
  }

  return ascii;
}

/*_____FILE METHODS_____*/

/* __localOpen()__
 *  - Given a file path, will open the file and return the file desciptor.
 *      (Adds fd to negfd LL)
 *
 *  Arguments:
 *    msgSize - the number of characters in the path name read from the client
 *    msg - the path name read from the client
 *    retMsgSize - a pointer to an integer where the size of the return msg
 *                  needs to be stored
 *
 *  Returns:
 *    - A pointer to a dynamically allocated string containing an error code
 *        and an ascii representation of the negated file descriptor
 *
 *    Input Message Format: "<File path>"
 *    Return Message Format: "<msgsize>,<error code>,<negfd>"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,<negfd>"
 */
char * localOpen(int msgSize, char * msg, int * retMsgSize){
  printf("Open():\nSize: %d\nData: %s\n\n", msgSize, msg);

  *retMsgSize = 9;
  char * ret = malloc((*retMsgSize)*sizeof(char));
  if(ret == NULL){
    threadError("ERROR Malloc Failure", __LINE__,0);
    return NULL;
  }

  *ret = '\0';
  strcpy(ret, "8,Open()");

  return ret; //Mitch - Do This
}

/* __localRead()__
 *  - Given a file desciptor and size n, will read n bytes from the corresponding
 *      file and return those bytes in a dynamically allocated string
 *
 *  Arguments:
 *    msgSize - the number of characters in the message read from the client
 *    msg - the ascii representation of "size n"
 *    negfd - the negated file desciptor (need to locate in LL to find real fd)
 *    retMsgSize - a pointer to an integer where the size of the return msg
 *                  needs to be stored
 *
 *  Returns:
 *    - A pointer to a dynamically allocated string containing an error code
 *        and an ascii representation of the n bytes of data that were read
 *
 *    Input Message Format: "<size n>"
 *    Return Message Format: "<msgsize>,<error code>,<read data>"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,<read data>"
 */
char * localRead(int msgSize, char * msg, int negfd, int * retMsgSize){
  printf("Read():\nNegFd: %d\nSize: %d\nData: %s\n\n",negfd, msgSize, msg);

  *retMsgSize = 9;
  char * ret = malloc((*retMsgSize)*sizeof(char));
  if(ret == NULL){
    threadError("ERROR Malloc Failure", __LINE__,0);
    return NULL;
  }

  *ret = '\0';
  strcpy(ret, "8,Read()");

  return ret; //Mitch - Do This
}

/* __localWrite()__
 *  - Given a file desciptor and a byte sequence, will write the bytes into the
 *      corresponding file and return an error code to signify success
 *
 *  Arguments:
 *    msgSize - the number of characters in the path name read from the client
 *    msg - the ascii representation of the byte sequence to write
 *    negfd - the negated file desciptor (need to locate in LL to find real fd)
 *    retMsgSize - a pointer to an integer where the size of the return msg
 *                  needs to be stored
 *
 *  Returns:
 *    - A pointer to a dynamically allocated string containing an error code
 *
 *    Input Message Format: "<byte sequence>"
 *    Return Message Format: "<msgsize>,<error code>,w"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,w"
 */
char * localWrite(int msgSize, char * msg, int negfd, int * retMsgSize){
  printf("Write():\nNegFd: %d\nSize: %d\nData: %s\n\n", negfd, msgSize, msg);

  *retMsgSize = 10;
  char * ret = malloc((*retMsgSize)*sizeof(char));
  if(ret == NULL){
    threadError("ERROR Malloc Failure", __LINE__,0);
    return NULL;
  }

  *ret = '\0';
  strcpy(ret, "9,Write()");

  return ret; //Mitch - Do This
}

/* __localClose()__
 *  - Given a file desciptor, it will close the associated local file
 *      (Removes fd from negfd LL)
 *
 *  Arguments:
 *    negfd - the negated file desciptor (need to locate in LL to find real fd)
 *    retMsgSize - a pointer to an integer where the size of the return msg
 *                  needs to be stored
 *
 *  Returns:
 *    - A pointer to a dynamically allocated string containing an error code
 *
 *    Return Message Format: "<msgsize>,<error code>,c"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,c"
 */
char * localClose(int negfd, int * retMsgSize){
  printf("Close():\nNegFd: %d\n\n", negfd);

  *retMsgSize = 10;
  char * ret = malloc((*retMsgSize)*sizeof(char));
  if(ret == NULL){
    threadError("ERROR Malloc Failure", __LINE__,0);
    return NULL;
  }

  *ret = '\0';
  strcpy(ret, "9,Close()");

  return ret; //Mitch - Do This
}

/*_____Thread Method_____*/

/* __clientHandler()__
 *  - Handles all the correspondence with a client for a single instance
 *
 *  Arguments:
 *    sock - the socket file descriptor needed to communicate with the client
 *
 *  Returns:
 *    N/A
 *
 *    Message format: "size,op,<permissions>,fd,data"
 */
void * clientHandler(void * sock) {
  char buffer[250]; //Buffer for reading input and writing output
  //Zero out buffer and read message into buffer
  bzero(buffer,250);

  //Create buffer to read Packet Size
  char * init_buffer = malloc(1);
  if(init_buffer == NULL) {
    threadError("ERROR: Malloc Failure, dropping Client", __LINE__, errno);
    close(*((int*)sock));
    free(sock);
    pthread_exit(0);
  }

  //Create and Initialize reading vars
  int status, amount, total_Size = 0;
  int offset = 0;
  char * numEnd = NULL;

  while(numEnd == NULL){ //While we still haven't found a comma
    total_Size += MIN_BUFF_SIZE;

    //Realloc to ensure you have enough room
    init_buffer = realloc(init_buffer, total_Size);
    if(init_buffer == NULL){
      threadError("ERROR: Realloc Failure, dropping Client", __LINE__, errno);
      close(*((int*)sock));
      free(sock);
      pthread_exit(0);
    }

    //Read bytes into init_buffer
    amount = MIN_BUFF_SIZE;
    status = 0;
    while(amount > 0) { //Ensure MIN_BUFF_SIZE bytes were read...
      status = read (*((int*) sock), init_buffer+offset, amount);
      if (status < 0){
        threadError("ERROR reading from socket", __LINE__, errno);
        free(init_buffer);
        close(*((int*)sock));
        free(sock);
        pthread_exit(0);
      }
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

  //Malloc for the rest of the message (includng the comma)
  char * msg = malloc(msgSize*sizeof(char));
  if(msg == NULL) {
    threadError("ERROR: Malloc Failure, dropping Client", __LINE__, errno);
    free(init_buffer);
    close(*((int*)sock));
    free(sock);
    pthread_exit(0);
  }

  int excess_len = total_Size - (numEnd - init_buffer);

  //Copy extra data already read
  msg = memcpy(msg, numEnd, excess_len);

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
      threadError("ERROR reading from socket", __LINE__, errno);
      free(init_buffer);
      free(msg);
      close(*((int*)sock));
      free(sock);
      pthread_exit(0);
    }
  }

  //Free the buffer now that we're done with it
  free(init_buffer);

  // _____READ META_DATA_____

  char * ptr = msg; //Message traversal variable
  char * returningMsg; //pointer to malloced returning Message
  int retMsgSize = 0;
  int fd = 0; //File Discriptor
  char op = '\0'; //Operation

  //read and decide op
  ptr++;
  op = *ptr;

  switch(op){
    case 'o': //Open
      ptr += 2;
      returningMsg = localOpen(msgSize - 3, ptr, &retMsgSize);
      break;

    case 'r': //Read
      ptr += 2;
      numEnd = tryNum(ptr, msgSize - 3); //read fd
      if(numEnd == NULL);
      *numEnd = '\0';
      fd = atoi(ptr);
      *numEnd = ',';
      returningMsg = localRead(msgSize - (numEnd - msg) - 1, numEnd + 1, fd, &retMsgSize);
      break;

    case 'w': //Write
      ptr += 2;
      numEnd = tryNum(ptr, msgSize - 3); //read fd
      if(numEnd == NULL);
      *numEnd = '\0';
      fd = atoi(ptr);
      *numEnd = ',';
      returningMsg = localWrite(msgSize - (numEnd - msg) - 1, numEnd + 1, fd, &retMsgSize);
      break;

    case 'c': //Close
      ptr += 2;
      numEnd = tryNum(ptr, msgSize - 3); //read fd
      if(numEnd == NULL);
      *numEnd = '\0';
      fd = atoi(ptr);
      *numEnd = ',';
      returningMsg = localClose(fd, &retMsgSize);
      break;

    default:
      threadError("ERROR Not an OP",__LINE__,errno);
      free(msg);
      close(*((int*)sock));
      free(sock);
      pthread_exit(0);
  }

  //Free msg after use
  free(msg);

  //Message creation failure
  if(returningMsg == NULL){
    free(returningMsg);
    close(*((int*)sock));
    free(sock);
    pthread_exit(0);
  }

  //Write the message back to the client
  status = write(*((int*) sock),returningMsg,retMsgSize);
  if (status < 0){
       threadError("ERROR writing to socket",__LINE__,errno);
       free(returningMsg);
       close(*((int*)sock));
       free(sock);
       pthread_exit(0);
  }

  //Free returningMsg after it is sent
  free(returningMsg);

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
     if (argc != 2)
        regError("ERROR No port provided",__LINE__,EINVAL);

     //Create a socket
     baseSock = socket(AF_INET, SOCK_STREAM, 0);
     if (baseSock < 0)
        regError("ERROR opening socket",__LINE__,errno);

     portNum = atoi(argv[1]); //Store port number

     //Build sockaddr_in struct (3 steps)
     bzero((char *) &serv_addr, sizeof(serv_addr)); //1)Zero out space
     serv_addr.sin_family = AF_INET; //2) Set addr family to AF_INET
     serv_addr.sin_port = htons(portNum); //3) Set up the port

     serv_addr.sin_addr.s_addr = INADDR_ANY; //Set to accept any IP

     //Sets up socket
     if (bind(baseSock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
          regError("ERROR on binding",__LINE__,errno);
     listen(baseSock,5); //Preps to accept clients (only holds 5 clients in queue)
     clilen = sizeof(cli_addr);

     while(1 == 1){
          //Wait for client
          passSock = accept(baseSock, (struct sockaddr *) &cli_addr, &clilen);
          if (passSock < 0)
               regError("ERROR on accept",__LINE__,errno);

          //Malloc to pass socket descriptor to cliHand()
          int * pass = malloc(sizeof(int));
          if (pass == NULL)
               regError("ERROR on malloc",__LINE__,errno);
          *pass = passSock; //Put it in pass

          //Begin thread creation
          pthread_t cliThread;
          pthread_attr_t cliAttr;

          //Initialize and set Attributes to deatached state to allow exit to end thread
          if(pthread_attr_init(&cliAttr) != 0)
            regError("ERROR on pthread_attr_init",__LINE__,errno);
          if(pthread_attr_setdetachstate(&cliAttr, PTHREAD_CREATE_DETACHED) != 0)
            regError("ERROR on pthread_attr_setdetachstate",__LINE__,errno);

          //Create thread and continute
          if(pthread_create(&cliThread, &cliAttr, cliHand, (void*) pass) != 0)
            regError("ERROR on pthread_create",__LINE__,errno);
     }

     close(baseSock);

     return 0;
}
