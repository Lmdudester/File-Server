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

typedef struct node {
        int fd; //file descriptor (the postivie one)
        struct node* next;
} node_t;


/*_____GLOBAL VARIABLES_____*/

node_t* front = NULL;
pthread_mutex_t mutexLL = PTHREAD_MUTEX_INITIALIZER;


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


/*_____NODE FUNCTIONS_____*/

/* Function: createNode
 * --------------------
 * creates a node that can be used in a linked list
 *
 *  fd: the fd for the node
 *
 *  returns: pointer to the node
 */
node_t * createNode(int fd){
        //Mallocs space for node
        node_t * newNode = malloc(sizeof(node_t));
        if (newNode == NULL) {
                fprintf(stderr, "Malloc failed.\n");
        }

        //Set fd to given fd, but negated
        (*newNode).fd = fd;

        //Set next to NULL
        (*newNode).next = NULL;

        return newNode;
}

/* Function: placeNode
 * --------------------
 * Places the given node in the linked list
 *
 * node: node to be placed
 *
 *  returns: 0 for success, -1 for error
 */
int placeNode (node_t* node){ //node_t* ins
        pthread_mutex_lock(&mutexLL);
        node_t * ins;
        if(front == NULL) { //If LL is empty
                ins = node;
                front = ins;
                pthread_mutex_unlock(&mutexLL);
                return 0;
        }

        int cmp = intComp((*node).fd,(*front).fd);

        if(cmp != 2) { //If ins is belongs at the front
                if(cmp == 0) { //INCREMENT
                        front = front;
                        pthread_mutex_unlock(&mutexLL);
                        return 0;
                }
                ins = node;
                (*ins).next = front;
                front = ins;
                pthread_mutex_unlock(&mutexLL);
                return 0;
        }

        node_t * ptr = (*front).next; //Track the current comparison node
        node_t * prv = front; //Keep the link to the previous one

        while(ptr != NULL) { //It belongs somwehere between two nodes
                cmp = intComp((*node).fd,(*ptr).fd);
                if(cmp != 2) {
                        if(cmp == 0) {
                                front = front;
                                pthread_mutex_unlock(&mutexLL);
                                return 0;
                        }
                        ins = node;
                        (*ins).next = ptr;
                        (*prv).next = ins;

                        front = front;
                        pthread_mutex_unlock(&mutexLL);
                        return 0;
                }
                prv = ptr;
                ptr = (*ptr).next;
        }

        //It belongs at the end of the LL
        ins = node;
        (*prv).next = ins;
        pthread_mutex_unlock(&mutexLL);
        return 0;
}

/* Function: removeNode
 * --------------------
 * Removes a node from the LL based off on the FD
 *
 * fd: file descriptor
 *
 *  returns: 0 for success, -1 for error
 */
int removeNode(int fd){
        pthread_mutex_lock(&mutexLL);
        //if the node to delete is the front
        if ((*front).fd == fd) {
                front = (*front).next;
                pthread_mutex_unlock(&mutexLL);
                return 0;
        }

        node_t* ptr = (*front).next;
        node_t* prv = front;

        while(ptr != NULL) {
                if ((*ptr).fd == fd) { //a match has been found
                        //if it is the last node
                        if((*ptr).next == NULL) {
                                (*prv).next = NULL;
                                front = front;
                                pthread_mutex_unlock(&mutexLL);
                                return 0;
                        } else { //in the middle
                                (*prv).next = (*ptr).next;
                                front = front;
                                pthread_mutex_unlock(&mutexLL);
                                return 0;
                        }
                } else { //a match hasn't been found
                        prv = ptr;
                        ptr = (*ptr).next;
                }
        }
        front = front;
        pthread_mutex_unlock(&mutexLL);
        return 0;
}

/* Function: findNode
 * --------------------
 * Finds a node from the LL based off on the FD
 *
 * fd: file descriptor
 *
 *  returns: -1 on not found, -2 on LL empty. 1 if found
 */
int findNode(int fd){
        pthread_mutex_lock(&mutexLL);
        if (front == NULL) {
                pthread_mutex_unlock(&mutexLL);
                return -2; //there's nothing in the list, nothing will be found
        }
        node_t* temp = front;

        int status = 0;
        while (temp != NULL && status != -1) {
                if ((*temp).fd == fd) {
                        pthread_mutex_unlock(&mutexLL);
                        return 1;
                } else if ((*temp).fd < fd) {
                        temp = (*temp).next; //move to the next node
                        status = 0;
                } else if ((*temp).fd > fd)
                        status = -1;
        }
        pthread_mutex_unlock(&mutexLL);

        if (status == 0) {
                return -1; //generic node not found error
        } else {
                return status; //the node wasn't found, return the actual status
        }
}

//FOR TESTING ONLY
void printLL(){
        pthread_mutex_lock(&mutexLL);
        node_t* temp = front;
        while(temp != NULL) {
                printf("Node: fd - '%i'\n", (*temp).fd);
                temp = (*temp).next;
        }
        printf("printLL complete\n");
        pthread_mutex_unlock(&mutexLL);
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
 *    Input Message Format: "<permissions>,<File path>"
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

        (*retMsgSize) = 0;

        switch(perm) {
          case 'r': //read-only
            fd = open(msg, O_RDONLY);
            break;

          case 'w': //write-only
            fd = open(msg, O_WRONLY);
            break;

          case 'b': //both
            fd = open(msg, O_RDWR);
            break;

          default: //data was corrupted
            fd = -1;
        }

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
                //okay, now it worked so the first thing we should do is to update the linked list
                placeNode(createNode(fd)); //creates and places a node

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
        int readNumLen = myatoi(msgSize, msg);

        char* buffer = malloc(sizeof(char)*readNumLen); //make a buffer to hold the string that is read
        int status = read(fd, buffer, (size_t)readNumLen);

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

        //staging - getting all of the parts of what we need
        //write needs the fd, so lets flip it
        int fd = -1 * negfd;
        int status = write(fd, msg, msgSize); //actually preform the write

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

        int closed;
        int found = findNode(fd);

        if(found != 1){ //Didn't find it
          errno = EBADF;
          closed = -1;
        } else {
          closed = close(fd);
        }

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

                //okay, now it worked so the first thing we should do is to update the linked list
                removeNode(fd); //removes node

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
 *    Message format: "size,op,fd,<permissions>,data"
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
