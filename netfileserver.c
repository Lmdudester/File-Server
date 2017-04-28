#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

#define PORTNUM 10897
#define MIN_BUFF_SIZE 5

/*_____STRUCTS_____*/

typedef struct _fdNode{
  int fd;
  struct _fdNode * next;
} fdNode;

typedef struct node {
        char * filename;
        fdNode * fds;
        //u - unrestricted, e/x - exclusive/exclusive & w is open, t - transaction
        char fmode;
        int wOpen;
        struct node * next;
} node_t;


/*_____GLOBAL VARIABLES_____*/

node_t* front = NULL;
pthread_mutex_t mutexLL = PTHREAD_MUTEX_INITIALIZER;

/*_____ADD NODE FUNCTIONS_____*/

/* Function: createNode
 * --------------------
 * - creates a node that can be used in a linked list
 *
 * filename: the path of the file assocated with the node and fd List
 * fd: the first fd for the node
 * mode: the mode in which the file is attempting to be opened
 *
 * returns: pointer to the node
 */
node_t * createNode(char * filename, int fd, char mode) {
  //Malloc for actual node
  node_t * newNode = malloc(sizeof(node_t));
  if(newNode == NULL)
    return NULL;

  //Place mode in
  (*newNode).fmode = mode;

  //Place filename in
  (*newNode).filename = malloc(sizeof(char)*(strlen(filename)+1));
  if((*newNode).filename == NULL)
    return NULL;
  (*((*newNode).filename)) = '\0';
  strcpy((*newNode).filename,filename);

  //Prepare file desciptor list
  (*newNode).fds = malloc(sizeof(fdNode));
  if((*newNode).fds == NULL)
    return NULL;
  (*((*newNode).fds)).fd = fd;

  //Set next to NULL
  (*newNode).next = NULL;

  //Set wOpen to 0
  (*newNode).wOpen = 0;

  return newNode;
}

/* Function: addfd
 * --------------------
 * - adds a fd to the file descriptor list
 *
 * ptr: the node to add the fd to
 * rdwr: the read/write privleges of the fd
 * oneOrTwo: the postion to place the node in ('1' or '2')
 *
 * returns: the opened fd if success, -1 if error
 */
int addfd(node_t * ptr, int rdwr, int index){
  //Attempt to open
  int fd = open((*ptr).filename, rdwr);
  if(fd == -1)
    return fd;

  //Malloc new fdNode
  fdNode * newfdNode = malloc(sizeof(fdNode));
  if(newfdNode == NULL)
    return -1;

  //Place fd in fdNode
  (*newfdNode).fd = fd;

  //Place it in the list
  if(index == 1){
    (*newfdNode).next = (*ptr).fds;
    (*ptr).fds = newfdNode;

    return fd;
  }

  fdNode * fdptr = (*ptr).fds;

  index -= 2;
  while(index != 0){
    fdptr = (*fdptr).next;
    index--;
  }

  (*newfdNode).next = (*fdptr).next;
  (*fdptr).next = newfdNode;

  return fd;
}

/* Function: findByName
 * --------------------
 * - Locates a node by the filename
 *
 * filename: the file path assocated with the node
 *
 * returns: a pointer to the node_t if found, NULL otherwise
 */
node_t * findByName(char * filename) {
  node_t * ptr = front;

  while(ptr != NULL){
    if(strcmp(filename, (*ptr).filename) == 0){ //They're the same
      return ptr;
    }
    //Otherwise continue
    ptr = (*ptr).next;
  }

  return NULL;
}

/* Function: handleAddNode
 * --------------------
 * - Given what open needs and a mode, will determine if the file can
 *     be opened and take the steps needed to open it
 *
 * filename: the file path assocated with the node
 * rdwr: the requested read/write privleges of the file
 * mode: 'u'/'e'/'t' to represent the file modes
 *
 * returns: the fd if opened, -1 otherwise
 */
int handleAddNode(char * filename, int rdwr, char mode) {
  node_t * ptr = findByName(filename);

  if(ptr == NULL){ //File is not open yet
    //Attempt to open file
    int fd = open(filename, rdwr);
    if(fd == -1)
      return -1;

    //Create a node with correct parameters
    switch(mode){
      case 'u': //Unrestricted
        if(rdwr != O_RDONLY) {
          ptr = createNode(filename, fd, 'u');
          (*ptr).wOpen += 1;
        } else {
          ptr = createNode(filename, fd, 'u');
        }
        break;

      case 'e': //Exclusive
        if(rdwr == O_RDONLY) //No write permission is open
          ptr = createNode(filename, fd, 'e');
        else //It has write permission open
          ptr = createNode(filename, fd, 'x');
        break;

      case 't': //Transaction
        ptr = createNode(filename, fd, 't');
        break;
    }

    //Node creation failure
    if(ptr == NULL)
      return -1;

    //Update front and return with success
    (*ptr).next = front;
    front = ptr;
    return fd;

  }

  //File is open. -- In what mode?
  switch((*ptr).fmode) {
    case 'u': //unrestricted
      //If its transaction mode
      if(mode == 't'){
        errno = EACCES;
        return -1;
      }

      //If its exclusive mode
      if(mode == 'e') {
        if((*ptr).wOpen == 1){ //Open in write
          if(rdwr == O_RDONLY){ //Trying to read
            (*ptr).fmode = 'x';
            return addfd(ptr, rdwr, 2);
          }
        } else if((*ptr).wOpen == 0){ //Not open in write
          if(rdwr == O_RDONLY){ //Trying to read
            (*ptr).fmode = 'e';
            return addfd(ptr, rdwr, 1);
          } //Trying to write
          (*ptr).fmode = 'x';
          return addfd(ptr, rdwr, 1);
        }
        errno = EACCES;
        return -1;
      }

      //Opening in write - 'u'
      if(rdwr != O_RDONLY){
        (*ptr).wOpen += 1;
        return addfd(ptr, rdwr, 1);
      }

      //Opening in read - 'u'
      return addfd(ptr, rdwr, (*ptr).wOpen + 1);

    case 'e': //exclusive
      //If its transaction mode
      if(mode == 't') {
        errno = EACCES;
        return -1;
      }

      if(rdwr == O_RDONLY){ //Add it
        return addfd(ptr, rdwr, 1);
      }
      //Set to 'x' and add it
      (*ptr).fmode = 'x';
      return addfd(ptr, rdwr, 1);

    case 'x': //exclusive & w is open
      //If its transaction mode
      if(mode == 't'){
        errno = EACCES;
        return -1;
      }

      if(rdwr == O_RDONLY){ //Add it
        return addfd(ptr, rdwr, 2);
      }

      errno = EACCES; //errno permission denied
      return -1;

    case 't': //transaction
      errno = EACCES; //errno permission denied
      return -1;
  }

  //Something went hoibly wrong
  return - 1;
}

/*_____DELETE NODE FUNCTIONS_____*/

/* Function: handleRemoveNode
 * --------------------
 * - Will locate a file descriptor and remove it (or the whole node)
 *    depending on what is required (also updates mode if needed)
 *
 * fd: the fd to be removed
 *
 * returns: 0 if it was removed, -1 otherwise
 */
int handleRemoveNode(int fd){
  if(fd < 0){
    errno = EINVAL;
    return -1;
  }

  //Locate by fd
  node_t * prv = NULL;
  node_t * ptr = front;
  fdNode * prvfd;
  fdNode * ptrfd;
  char boole = 'f';
  int fdNum = 1;

  while(ptr != NULL){
    ptrfd = (*ptr).fds;
    prvfd = NULL;

    while(ptrfd != NULL){
      if((*ptrfd).fd == fd){
        boole = 't';
        break;
      }

      prvfd = ptrfd;
      ptrfd = (*ptrfd).next;
      fdNum++;
    }
    if(boole == 't')
      break;

    prv = ptr;
    ptr = (*ptr).next;
    fdNum = 1;
  }

  if(ptr == NULL) //Didn't find it
    return -1;

  if(close((*ptrfd).fd) == -1) //Close it
    return -1;

  //If it was open in write mode, adjust the counter
  if((*ptr).fmode == 'u'){
    if(fdNum <= (*ptr).wOpen){
      (*ptr).wOpen -= 1;
    }
  }

  if(prvfd == NULL) { //Its the first fd in the list
    if((*ptrfd).next == NULL){ //Its the only fd in the list (NODE DELETE)
      if(prv == NULL){ //Update front
        free(ptrfd);
        free((*ptr).filename);
        front = (*ptr).next;
        free(ptr);
        return 0;

      } //No front Update
      free(ptrfd);
      free((*ptr).filename);
      (*prv).next = (*ptr).next;
      free(ptr);
      return 0;

    } //There are fds after it (no node delete) (update fds) - first fd

    //Alter filemode for exclusive
    if((*ptr).fmode == 'x')
      (*ptr).fmode = 'e';

    (*ptr).fds = (*ptrfd).next;
    free(ptrfd);
    return 0;

  } //Its not the first fd in the list (no node delete)
  (*prvfd).next = (*ptrfd).next;
  free(ptrfd);
  return 0;

}

/*_____LOCATE FD FUNCTIONS_____*/

/* Function: inList
 * --------------------
 * - Will locate a file descriptor and tell you its there if it is
 *
 * fd: the fd to be found
 *
 * returns: 0 if it was found, -1 otherwise
 */
int inList(int fd){
  node_t * ptr = front;

  while(ptr != NULL){
    fdNode * fdPtr = (*ptr).fds;
    while(fdPtr != NULL){
      if((*fdPtr).fd == fd)
        return 0;

      fdPtr = (*fdPtr).next;
    }
    ptr = (*ptr).next;
  }
  return -1;
}

/*_____HELPER FUNCTIONS FOR LOCAL FUNCTIONS_____*/

/* Function: intComp
 * --------------------
 * Given two ints consiting of alphanumeric characters, intComp compares them
 * and returns
 *
 *  int1, int2: the two ints being compared
 *
 *  returns:
 *       1 if int1 < int2 - int1 comes before int2
 *       0 if int1 = int2 - int1 is the same as int2
 *       2 if int1 > int2 - int1 comes after int2
 */
int intComp(int int1, int int2){
        if(int1 < int2) {
                return 1;
        } else if (int1 > int2) {
                return 2;
        } else {
                return 0;
        }
}

/* __intLength()__
 *  - Determines the number of chars needed to represent an int
 *
 *  Arguments:
 *    num - the int in question
 *
 *  Returns:
 *    - An int containing the number of chars needed to represent
 *        the argument int
 */
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

/* __myatoi()__
 *  - Turns an ascii value of a given length into an int
 *
 *  Arguments:
 *    msgSize - length of the number in characters
 *    msg - the number in ascii form (leftmost digit)
 *
 *  Returns:
 *    - int representation for the ascii values given
 */
int myatoi(int msgSize, char * msg){
        int ret = 0;

        int i, temp;
        int powerOf10 = 1;

        for (i = msgSize-1; i >= 0; i--) {
                if(msg[i] == '-'){
                    ret *= -1;
                      break;
                }
                temp = msg[i] - '0';
                //printf("%c - temp is %i\n", msg[i], temp);
                ret += temp * powerOf10;
                powerOf10 *= 10;
        }

        return ret; //return the atoi version of the number.
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
        while(*ascii != ',') {
                if(max_size == 0)
                        return NULL;
                ascii++;
                max_size--;
        }

        return ascii;
}


/*_____ERROR FUNCTIONS_____*/

/* __regError()__
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
 *    Input Message Format: "<permissions>,<fmode>,<File path>"
 *    Return Message Format: "<msgsize>,<error code>,<negfd>"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,<negfd>"
 */
char * localOpen(int msgSize, char * msg, int * retMsgSize){
        //declare all of the parts of the return message
        int retErrorCode;
        size_t len;
        int newMsgSize, fd;

        //Read RDWR Permissions for client
        char perm = *msg;
        msg += 2;
        msgSize -= 2;

        //Read fmode from message
        char fmode = *msg;
        msg += 2;
        msgSize -= 2;

        (*retMsgSize) = 0;

        //Lock to interact with LL
        pthread_mutex_lock(&mutexLL);

        switch(perm) {
          case 'r': //read-only
            fd = handleAddNode(msg, O_RDONLY, fmode);
            //open(msg, O_RDONLY);
            break;

          case 'w': //write-only
            fd = handleAddNode(msg, O_WRONLY, fmode);
            //open(msg, O_WRONLY);
            break;

          case 'b': //both
            fd = handleAddNode(msg, O_RDWR, fmode);
            //open(msg, O_RDWR);
            break;

          default: //data was corrupted
            fd = -1;
        }

        //Unlock, done with LL
        pthread_mutex_unlock(&mutexLL);

        //TODO wrap the malloc statements in an if to catch any errors
        if(fd < 0) { //open has encountered an error

                //let's set the returnMsgSize to 0 in case this isnt the first return
                newMsgSize = 0;
                len = 0;

                //first, we know that a return message will always have three commas in it. Let's add 3 to the return counter
                newMsgSize += (sizeof(char) * 4); //now we don't have to worry about the commas being counted anymore

                //lets get the return error
                retErrorCode = errno; //set the error code to the errno value
                newMsgSize += intLength(retErrorCode); //increment the msgsize counter by the length of the error

                //now lets work on the fd to re returned. Since this is an error case, we want a fd that will clearly indicate and error.
                //since it's supposed to be negative, if we set it as a postive fd, it will be pretty clear that there was an error.
                char retNegFd = '1';
                //we added another character to the return message, so lets increment the counter
                newMsgSize += (sizeof(char));

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc(newMsgSize + intLength(newMsgSize)); //the return message is msgsize long, plus the length of retMsgSize

                (*retMsgSize) = newMsgSize + intLength(newMsgSize); //Set retMsgSize correctly

                //the first thing in the message is the msgsize, so let's add that
                //however, msgsize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", newMsgSize);
                size_t len = intLength(newMsgSize);

                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                retMsg[len] = ',';
                len++;
                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                char ec[intLength(retErrorCode)];
                sprintf(ec, "%i", retErrorCode);
                memcpy(retMsg + len, ec, intLength(retErrorCode));
                len += intLength(retErrorCode);
                //now we have to add a comma
                retMsg[len] = ',';
                len++;
                //finally, we have to append the fd, which is thankfully already a char
                memcpy(retMsg + len, &retNegFd, 1);
                len++;

                retMsg[len] = ',';
                retMsg[len+1] = 'o';

                return retMsg;
        } else {
                //let's set the newMsgSize to 0 in case this isnt the first return
                newMsgSize = 0;
                len = 0;

                //now we have to make the return message.
                //first, we know that a return message will always have three commas in it. Let's add 3 to the return counter
                newMsgSize += (sizeof(char) * 4); //now we don't have to worry about the commas being counted anymore
                // printf("msgSize is %i\n", newMsgSize);

                //lets set the return error, which will be 0 to indicate a success
                newMsgSize += (sizeof(char)); //increment the newMsgSize counter

                //we added more characters to the return message, so lets increment the counter. Remember to include room for the negative sign.
                newMsgSize += (sizeof(char) * (intLength(fd*-1)));
                // printf("msgSize is %i\n", newMsgSize);

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc((sizeof(char) * newMsgSize) + (sizeof(char) * intLength(newMsgSize))); //the return message is retMsgSize long, plus the length of retMsgSize

                (*retMsgSize) = (sizeof(char) * newMsgSize) + (sizeof(char) * intLength(newMsgSize)); //Set retMsgSize correctly

                //the first thing in the message is the newMsgSize, so let's add that
                //however, newMsgSize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", newMsgSize);
                len += intLength(newMsgSize);
                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                retMsg[len] = ',';
                retMsg[len+1] = '0';
                retMsg[len+2] = ',';
                len += 3;

                //finally, we have to append the fd, which is thankfully already a char*
                char tempFd[intLength(fd*-1)];
                sprintf(tempFd, "%d", (fd*-1));
                memcpy(retMsg+len, tempFd, intLength(fd*-1));
                len += intLength(fd*-1);

                retMsg[len] = ',';
                retMsg[len+1] = 'o';

                return retMsg;
        }
}

/* __localRead()__
 *  - Given a file desciptor and size n, will read n bytes from the corresponding
 *      file and return those bytes in a dynamically allocated string
 *
 *  Arguments:
 *    msgSize - the number of characters in the message read from the client. Clarfication: the length of msg
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
 *    Return Message Format: "<newMsgSize>,<read ret>,<errno>,<read data>"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<read ret>,<errno>,<read data>"
 */
char * localRead(int msgSize, char * msg, int negfd, int * retMsgSize){
        //let's gather what we need
        int fd = -1 * negfd;
        int status;

        int readNumLen = myatoi(msgSize, msg);

        char * buffer = malloc(sizeof(char)*readNumLen); //make a buffer to hold the string that is read

        //Lock to interact with LL
        pthread_mutex_lock(&mutexLL);

        status = inList(fd);

        //Unlock, done with LL
        pthread_mutex_unlock(&mutexLL);

        if(status == 0)
          status = read(fd, buffer, (size_t)readNumLen);

        int newMsgSize = 0; //initalizes newMsgSize

        if (status <= 0) { //read failed - or read nothing
                free(buffer);

                int errorcode = errno;
                int len = 0;

                newMsgSize += intLength(errorcode); //<errno>
                newMsgSize += intLength(status); //<read ret>
                newMsgSize += 4; //, , , 0

                char * retMsg = malloc(sizeof(char) * (intLength(newMsgSize) + newMsgSize));

                (*retMsgSize) = intLength(newMsgSize) + newMsgSize;

                sprintf(retMsg, "%i", newMsgSize);
                retMsg[intLength(newMsgSize)] = ',';
                len = intLength(newMsgSize) + 1;

                char temp1[intLength(status)];
                sprintf(temp1, "%i", status);
                memcpy(retMsg+len, temp1, intLength(status));
                retMsg[len+intLength(newMsgSize)] = ',';
                len += intLength(status) + 1;

                char temp2[intLength(errorcode)];
                sprintf(temp2, "%i", errorcode);
                memcpy(retMsg+len, temp2, intLength(errorcode));
                len += intLength(errorcode);

                retMsg[len] = ',';
                retMsg[len+1] = 'r';

                return retMsg;
        } else {
                //printf("status is: %i\n", status);
                int errorcode = errno;
                int len = 0;

                newMsgSize = intLength(errorcode) + intLength(status) + status + 3;

                //allocates space for the number of read bytes
                char* retMsg = malloc(sizeof(char)*(newMsgSize + intLength(newMsgSize)));

                (*retMsgSize) = newMsgSize + intLength(newMsgSize);

                sprintf(retMsg, "%i", newMsgSize); //<newMsgSize>,
                retMsg[intLength(newMsgSize)] = ',';
                len = intLength(newMsgSize) + 1;

                char temp1[intLength(status)]; //<read ret>,
                sprintf(temp1, "%i", status);
                memcpy(retMsg+len, temp1, intLength(status));
                retMsg[len+intLength(status)] = ',';
                len += intLength(status) + 1;

                retMsg[len] = '0';
                retMsg[len+1] = ','; //<errno>,
                len += 2;

                memcpy(retMsg+len, buffer, status); //data

                free(buffer);

                return retMsg;
        }
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
 *    Return Message Format: "<newMsgSize>,<write ret>,<error code>,w"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<write ret>,<error code>,w"
 */
char * localWrite(int msgSize, char * msg, int negfd, int * retMsgSize){
        int newMsgSize = 0;
        int status = 0;

        //staging - getting all of the parts of what we need
        //write needs the fd, so lets flip it
        int fd = -1 * negfd;

        //Lock to interact with LL
        pthread_mutex_lock(&mutexLL);

        status = inList(fd);

        //Unlock, done with LL
        pthread_mutex_unlock(&mutexLL);

        if(status == 0)
          status = write(fd, msg, msgSize); //actually preform the write

        if (status <= 0) {
                int errorcode = errno;
                int len = 0;

                newMsgSize = intLength(errorcode) + intLength(status) + 4;

                char * retMsg = malloc(sizeof(char) * (intLength(newMsgSize) + newMsgSize));

                (*retMsgSize) = intLength(newMsgSize) + newMsgSize;

                sprintf(retMsg, "%i", newMsgSize);
                retMsg[intLength(newMsgSize)] = ',';
                len += intLength(newMsgSize) + 1;

                char temp1[intLength(status)];
                sprintf(temp1, "%i", status);
                memcpy(retMsg+len, temp1, intLength(status));
                retMsg[len+intLength(status)] = ',';
                len += intLength(status) + 1;

                char temp2[intLength(errorcode)];
                sprintf(temp2, "%i", errorcode);
                memcpy(retMsg+len, temp2, intLength(errorcode));
                retMsg[len+intLength(errorcode)] = ',';
                len += intLength(errorcode) + 1;

                retMsg[len] = 'w';

                return retMsg;
        } else { //"<newMsgSize>,<write ret>,<error code>,w"

                //now we are working with the case where the write was successful, in which case the message will always be the same
                //we know how long it should be, so we can mallloc it directly
                int len = 0;

                newMsgSize = intLength(status) + 5; //len,status,0,w

                char* retMsg = malloc(sizeof(char) * (intLength(newMsgSize) + newMsgSize));

                (*retMsgSize) = intLength(newMsgSize) + newMsgSize;

                sprintf(retMsg, "%i", newMsgSize);
                retMsg[intLength(newMsgSize)] = ',';
                len += intLength(newMsgSize) + 1;

                char temp1[intLength(status)];
                sprintf(temp1, "%i", status);
                memcpy(retMsg+len, temp1, intLength(status));
                retMsg[len+intLength(status)] = ',';
                retMsg[len+intLength(status)+1] = '0';
                retMsg[len+intLength(status)+2] = ',';
                retMsg[len+intLength(status)+3] = 'w';

                return retMsg;
        }
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
        //since the stored fd are postive, let's flip the given fd
        int fd = -1 * negfd;
        int newMsgSize = 0;
        size_t len;

        //Lock to interact with LL
        pthread_mutex_lock(&mutexLL);

        int closed = handleRemoveNode(fd);

        //Unlock, done with LL
        pthread_mutex_unlock(&mutexLL);

        //let's first see if the file exists
        if (closed == -1) { //An error occured
                len = 0;

                //first, we know that a return message will always have two commas in it and a c. Let's add 2 to the return counter
                newMsgSize = 3; //now we don't have to worry about the commas being counted anymore

                //lets get the return error
                int retErrorCode = errno; //set the error code to the errno value
                newMsgSize += intLength(retErrorCode); //increment the retMsgSize counter by the length of the error

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc(sizeof(char)*(newMsgSize + intLength(newMsgSize))); //the return message is retMsgSize long, plus the length of retMsgSize

                (*retMsgSize) = newMsgSize + intLength(newMsgSize); //Set retMsgSize

                //the first thing in the message is the retMsgSize, so let's add that
                //however, retMsgSize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", newMsgSize);
                len += intLength(newMsgSize);

                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                retMsg[len] = ',';
                len++;

                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                char ec[intLength(retErrorCode)];
                sprintf(ec, "%i", retErrorCode);
                memcpy(retMsg + len, ec, intLength(retErrorCode));
                len += intLength(retErrorCode);

                //now we have to add a comma and the c
                retMsg[len] = ',';
                retMsg[len+1] = 'c';

                return retMsg;
        } else {
                len = 0;

                //let's set the returnMsgSize to 0 in case this isnt the first return
                (*retMsgSize) = 5;

                char* retMsg = malloc(5*sizeof(char)); //the return message is retMsgSize long, plus the length of retMsgSize
                //the first thing in the message is the retMsgSize, so let's add that
                //however, retMsgSize is an int right now and we need it as a char*, so let's cast it
                memcpy(retMsg, "4,0,c", 5);
                return retMsg;
        }
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
 *    Message format: "size,op,fd,<permissions>,<mode>,data"
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

        while(numEnd == NULL) { //While we still haven't found a comma
                total_Size += MIN_BUFF_SIZE;

                //Realloc to ensure you have enough room
                init_buffer = realloc(init_buffer, total_Size);
                if(init_buffer == NULL) {
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
                        if (status < 0) {
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

        switch(op) {
        case 'o': //Open
                ptr += 2;
                returningMsg = localOpen(msgSize - 3, ptr, &retMsgSize);
                break;

        case 'r': //Read
                ptr += 2;
                numEnd = tryNum(ptr, msgSize - 3); //read fd
                if(numEnd == NULL) {
                        threadError("ERROR No file desciptor", __LINE__, EBADMSG);
                        free(msg);
                        close(*((int*)sock));
                        free(sock);
                        pthread_exit(0);
                }
                *numEnd = '\0';
                fd = atoi(ptr);
                *numEnd = ',';
                returningMsg = localRead(msgSize - (numEnd - msg) - 1, numEnd + 1, fd, &retMsgSize);
                break;

        case 'w': //Write
                ptr += 2;
                numEnd = tryNum(ptr, msgSize - 3); //read fd
                if(numEnd == NULL) {
                        threadError("ERROR No file desciptor", __LINE__, EBADMSG);
                        free(msg);
                        close(*((int*)sock));
                        free(sock);
                        pthread_exit(0);
                }
                *numEnd = '\0';
                fd = atoi(ptr);
                *numEnd = ',';
                returningMsg = localWrite(msgSize - (numEnd - msg) - 1, numEnd + 1, fd, &retMsgSize);
                break;

        case 'c': //Close
                ptr += 2;
                numEnd = tryNum(ptr, msgSize - 3); //read fd
                if(numEnd == NULL) {
                        threadError("ERROR No file desciptor", __LINE__, EBADMSG);
                        free(msg);
                        close(*((int*)sock));
                        free(sock);
                        pthread_exit(0);
                }
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
        if(returningMsg == NULL || retMsgSize <= 4) {
                close(*((int*)sock));
                free(sock);
                pthread_exit(0);
        }

        //Write the message back to the client
        status = write(*((int*) sock),returningMsg,retMsgSize);
        if (status < 0)
                threadError("ERROR writing to socket",__LINE__,errno);

        //Free returningMsg after it is sent
        free(returningMsg);

        close(*((int*)sock));
        free(sock);
        pthread_exit(0);
}


/*_____MAIN_____*/

int main(int argc, char *argv[]) {
        int baseSock, passSock, portNum; //Info required for server
        socklen_t clilen; //Client length -> needed to accept()

        void* (*cliHand)(void*) = clientHandler; //Threading method

        struct sockaddr_in serv_addr, cli_addr; //For setting up sockets

        //Create a socket
        baseSock = socket(AF_INET, SOCK_STREAM, 0);
        if (baseSock < 0)
                regError("ERROR opening socket",__LINE__,errno);

        int one = 1;
        if (setsockopt(baseSock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
            regError("ERROR setsockopt Failure",__LINE__,errno);

        portNum = PORTNUM; //Store port number

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

        while(1 == 1) {
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
