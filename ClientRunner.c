#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define PORTNUM 10897
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define MIN_BUFF_SIZE 5

static struct hostent *verifiedServer = NULL;

/*____FUNCTION HELPERS____*/

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
        while(*ascii != ',') {
                if(max_size == 0)
                        return NULL;
                ascii++;
                max_size--;
        }

        return ascii;
}

//returns the length of an int
int intLength (int num){
        int count = 0;
        if(num < 0){
          count++;
          num *= -1;
        }
        if(num == 0)
          return 1;
        while (num != 0) {
                num /= 10;
                count++;
        }
        return count;
}

//DONE
int getSock(){
  if(verifiedServer == NULL)
    return -1;

  int sockfd, portno; //Info required for client
  struct sockaddr_in serv_addr; //Info for setting up socket

  //Get port number
  portno = PORTNUM;

  //Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
      return -1;

  //Build sockaddr_in struct (4 steps)
  bzero((char *) &serv_addr, sizeof(serv_addr)); //1) Zero out space

  serv_addr.sin_family = AF_INET; //2) Set addr family to AF_INET

  serv_addr.sin_port = htons(portno); //3) Set up the port

  bcopy((char *)verifiedServer->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        verifiedServer->h_length); //4) Copy representation of IP from hostent struct to sockaddr_in


  //Try to connect, if connect fails, ERRNO is set
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
      return -1;

  return sockfd;
}

char * readFromServer(int sockfd, int * retSize){
  //Create buffer to read Packet Size
  char * init_buffer = malloc(1);
  if(init_buffer == NULL)
          return NULL;

  //Create and Initialize reading vars
  int status, amount, total_Size = 0;
  int offset = 0;
  char * numEnd = NULL;

  while(numEnd == NULL) { //While we still haven't found a comma
          total_Size += MIN_BUFF_SIZE;

          //Realloc to ensure you have enough room
          init_buffer = realloc(init_buffer, total_Size);
          if(init_buffer == NULL)
                  return NULL;


          //Read bytes into init_buffer
          amount = MIN_BUFF_SIZE;
          status = 0;
          while(amount > 0) { //Ensure MIN_BUFF_SIZE bytes were read...
                  status = read (sockfd, init_buffer+offset, amount);
                  if (status < 0) {
                          free(init_buffer);
                          return NULL;
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
          free(init_buffer);
          return NULL;
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
          status = read (sockfd, msg+offset, amount);
          if (status >= 0) {
                  amount -= status;
          } else {
                  free(init_buffer);
                  free(msg);
                  return NULL;
          }
  }

  //Free the buffer now that we're done with it
  free(init_buffer);

  (*retSize) = msgSize;

  return msg;
}

//DONE
void testStuff(){
  int status, sockfd;
  char buffer[250]; //For what to send to server

  sockfd = getSock();
  if(sockfd < 0){
    printf("ERROR Use netinit first\n");
    return;
  }

  //---- Successfully Connected ----//
  printf("Enter Message: ");
  bzero(buffer,250);
  fgets(buffer, 250,stdin);

  //Write this buffer to the socket
  status = write(sockfd,buffer,250);
  if (status < 0){
       printf("ERROR writing to socket\n");
       return;
  }

  //Zero out the buffer for a read
  bzero(buffer,250);

  //Read from the socket into the buffer
  status = read(sockfd,buffer,250);
  if (status < 0){
       printf("ERROR reading from socket\n");
       return;
  }

  //Close the socket using sockfd
  close(sockfd);

  //Print read information
  printf("Ret Msg: %s\n", buffer);
}

/*____NET-FUNCTIONS____*/

//DONE
int netserverinit(char * hostname){
  verifiedServer = gethostbyname(hostname);
  if (verifiedServer == NULL) {
      h_errno = HOST_NOT_FOUND;
      return -1;
  }

  return 0;
}

//DONE
int netopen(const char *pathname, int flags){
  //No path name
  if(pathname == NULL && strlen(pathname) > 0){
    errno = ENOENT;
    return -1;
  }

  //Improper flags
  if(flags < 0 || flags > 2){
    errno = EPERM;
    return -1;
  }

  //Set up the size variable
  int sendSize = strlen(pathname) + 6;
  int len = 0;

  //Malloc the correct size for the message
  char * sendMsg = malloc(sizeof(char)*(sendSize + intLength(sendSize)));
  if(sendMsg == NULL) //Errno set by malloc
    return -1;

  //Print in the size
  sprintf(sendMsg, "%i", sendSize);
  len += intLength(sendSize);

  //Print open command
  sendMsg[len] = ',';
  sendMsg[len+1] = 'o';
  sendMsg[len+2] = ',';

  //Determine permission status and enter it
  if(flags == O_RDONLY){
    sendMsg[len+3] = 'r';
  } else if(flags == O_WRONLY){
    sendMsg[len+3] = 'w';
  } else {
    sendMsg[len+3] = 'b';
  }
  sendMsg[len+4] = ',';
  len += 5;

  //Copy file path will NULL byte
  memcpy(sendMsg+len, pathname, strlen(pathname) + 1);

  //Correct size to invlude ascii size
  sendSize += intLength(sendSize);

  //Open a socket
  int sockfd = getSock();
  int status;

  //Write to the server
  status = write(sockfd,sendMsg,sendSize);
  if (status < 0){
    errno = TRY_AGAIN;
    return -1;
  }

  //We're done with sendMsg so free it
  free(sendMsg);

  //Read from the server
  int msgSize;
  char * msg = readFromServer(sockfd, &msgSize); //Get malloced message
  if(msg == NULL) //Errno set by readFromServer
    return -1;

  //printf("Message: %s, Size: %d\n", msg, msgSize);

  //Set errno
  char * ptr = msg + 1;
  char * end = tryNum(ptr, msgSize-1);
  if(end == NULL){
    errno = TRY_AGAIN;
    return -1;
  }
  *end = '\0';
  errno = atoi(ptr);
  *end = ',';

  //Get return value
  msgSize -= (end - ptr) - 1;
  ptr = end + 1;
  end = tryNum(ptr, msgSize);
  if(end == NULL){
    errno = TRY_AGAIN;
    return -1;
  }
  *end = '\0';
  int ret = atoi(ptr);
  *end = ',';

  return ret;
}

int main(int argc, char *argv[]) {
    if(netserverinit(argv[1]) == 0){
      printf("Host Does Exist.\n");
      testStuff();
    } else {
      printf("Error, bad host");
    }

    /*
    //Test netopen
    printf("fd: %d\n", netopen("./tests/test1.txt",O_RDWR));
    */
    return 0;
}
