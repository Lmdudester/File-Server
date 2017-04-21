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

#define MIN_BUFF_SIZE 5
typedef struct node {
        int fd; //file descriptor (the postivie one)
        struct node* next;
} node_t;

node_t* front = NULL;
pthread_mutex_t mutexLL = PTHREAD_MUTEX_INITIALIZER;

/*
 * Function: intComp
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

/*
 * Function: createNode
 * --------------------
 * creates a node that can be used in a linked list
 *
 * token: the token that is to be placed
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

/*
 * Function: placeNode
 * --------------------
 * Places the given node in the linked list
 *
 * word: the token
 * filename: the corresponding file name
 * front: pointer to the head of the list
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

/**
 * Removes a node from the LL based off on the FD
 * @param  fd    file descriptor
 * @param  front pointer to the front of the LL
 * @return       0 on success, -1 on failure
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

//return -1 on not found, -2 on LL empty. 1 if found
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
        while(*ascii != ',') {
                if(max_size == 0)
                        return NULL;
                ascii++;
                max_size--;
        }

        return ascii;
}

/*_____FILE METHODS_____*/
//returns the length of an int
int intLength (int num){
        int count = 0;
        while (num != 0) {
                num /= 10;
                count++;
        }
        return count;
}

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

        //declare all of the parts of the return message
        int retErrorCode;
        char* retNegFd;

        int fd = open(msg, O_RDWR);

        //TODO wrap the malloc statements in an if to catch any errors
        if(fd < 0) { //open has encountered an error

                //let's set the returnMsgSize to 0 in case this isnt the first return
                (*retMsgSize) = 0;

                //first, we know that a return message will always have two commas in it. Let's add 2 to the return counter
                (*retMsgSize) += (sizeof(char) * 2); //now we don't have to worry about the commas being counted anymore

                //lets get the return error
                retErrorCode = errno; //set the error code to the errno value
                (*retMsgSize) += intLength(retErrorCode); //increment the retMsgSize counter by the length of the error

                //now lets work on the fd to re returned. Since this is an error case, we want a fd that will clearly indicate and error.
                //since it's supposed to be negative, if we set it as a postive fd, it will be pretty clear that there was an error.
                retNegFd = malloc(sizeof(char) * 2); //malloc one byte for a fd of '1', and a byte for the null byte
                retNegFd[0] = '1'; //set the first byte as the positive fd 1
                retNegFd[1] = '\0'; //set the last byte as the null byte
                //we added another character to the return message, so lets increment the counter
                (*retMsgSize) += (sizeof(char));

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc((*retMsgSize) + intLength((*retMsgSize))); //the return message is retMsgSize long, plus the length of retMsgSize
                //the first thing in the message is the retMsgSize, so let's add that
                //however, retMsgSize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", (*retMsgSize));
                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                strcat(retMsg, ",");
                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                char* tempErrorCode = malloc(sizeof(int) * intLength(retErrorCode) + sizeof(char)); //don't forget to add room for a null byte
                sprintf(tempErrorCode, "%i", retErrorCode);
                //now we can concat that
                strcat(retMsg, tempErrorCode);
                //now we have to add a comma
                strcat(retMsg, ",");
                //finally, we have to append the fd, which is thankfully already a char*
                strcat(retMsg, retNegFd);

                //time to be a responsible programmer and clean up what we malloced
                free(retNegFd);
                free(tempErrorCode);
                return retMsg;
        } else {
                //okay, now it worked so the first thing we should do is to update the linked list
                placeNode(createNode(fd)); //creates and places a node

                //let's set the returnMsgSize to 0 in case this isnt the first return
                (*retMsgSize) = 0;

                //now we have to make the return message.
                //first, we know that a return message will always have two commas in it. Let's add 2 to the return counter
                (*retMsgSize) += (sizeof(char) * 2); //now we don't have to worry about the commas being counted anymore

                //lets set the return error, which will be 0 to indicate a success
                retErrorCode = 0;
                (*retMsgSize) += (sizeof(char)); //increment the retMsgSize counter

                //now lets work on the fd to be returned. convert the fd to a string
                retNegFd = malloc((sizeof(char) * intLength(fd)) + sizeof(char));                  //make room for the fd and a null byte
                sprintf(retNegFd, "-%i", fd);
                //we added more characters to the return message, so lets increment the counter. Remember to include room for the negative sign.
                (*retMsgSize) += (sizeof(char) * (intLength(fd) + 1));

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc((sizeof(char) * (*retMsgSize)) + (sizeof(char) * intLength((*retMsgSize)))); //the return message is retMsgSize long, plus the length of retMsgSize
                //the first thing in the message is the retMsgSize, so let's add that
                //however, retMsgSize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", (*retMsgSize));
                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                strcat(retMsg, ",");
                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                char* tempErrorCode = malloc(sizeof(int) * intLength(retErrorCode) + sizeof(char)); //don't forget to add room for a null byte
                sprintf(tempErrorCode, "%i", retErrorCode);
                //now we can concat that
                strcat(retMsg, tempErrorCode);
                //now we have to add a comma
                strcat(retMsg, ",");
                //finally, we have to append the fd, which is thankfully already a char*
                strcat(retMsg, retNegFd);

                //time to be a responsible programmer and clean up what we malloced
                free(retNegFd);
                free(tempErrorCode);
                return retMsg;
        }
}

int myatoi(int msgSize, char * msg){
        int ret = 0;

        int i, temp;
        int powerOf10 = 1;

        for (i = msgSize-1; i >= 0; i--) {
                temp = msg[i] - '0';
                printf("%c - temp is %i\n", msg[i], temp);
                ret += temp * powerOf10;
                powerOf10 *= 10;
        }

        return ret; //return the atoi version of the number.
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
 *    Return Message Format: "<msgsize>,<error code>,<read data>"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,<read data>"
 */
char * localRead(int msgSize, char * msg, int negfd, int * retMsgSize){
        //let's gather what we need
        int fd = -1 * negfd;
        char* buffer = malloc(msgSize); //make a buffer to hold the string that is read
        int status = read(fd, buffer, (size_t)msgSize);

        (*retMsgSize) += 2; //takes care of the commas

        if (status < 0) { //read failed
                free(buffer);
                int errorcode = errno;
                (*retMsgSize) += intLength(errorcode);
                char * retMsg = malloc(sizeof(char) * (intLength((*retMsgSize)) + (*retMsgSize)));
                sprintf(retMsg, "%i", (*retMsgSize));
                strcat(retMsg, ",");
                char* temp = malloc(intLength(errorcode));
                sprintf(temp, "%i", errorcode);
                strcat(retMsg, temp);
                strcat(retMsg, ",0");
                free(temp);
                return retMsg;
        } else {
                char* retMsg = malloc(sizeof(char) * status); //shrinks the allocated space to the number of read bytes
                (*retMsgSize) += status;
                (*retMsgSize) += strlen(buffer);
                sprintf(retMsg, "%i", (*retMsgSize));
                strcat(retMsg, ",0,");
                strcat(retMsg, buffer);
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
 *    Return Message Format: "<msgsize>,<error code>,w"
 *
 *      *Note: retMsgSize is the number of bytes malloced for returned msg
 *              <msgsize> is the number of bytes in ",<error code>,w"
 */
char * localWrite(int msgSize, char * msg, int negfd, int * retMsgSize){
        //zero out the return length counter
        (*retMsgSize) = 0;
        //staging - getting all of the parts of what we need
        //write needs the fd, so lets flip it
        int fd = -1 * negfd;
        int status = write(fd, &msg, (size_t)strlen(msg)); //actually preform the write
        (*retMsgSize) += 3; //account for the 2 commas and a 'w'
        if (status == -1) {
                int errorcode = errno;
                (*retMsgSize) += intLength(errorcode);
                char * retMsg = malloc(sizeof(char) * (intLength((*retMsgSize)) + (*retMsgSize)));
                sprintf(retMsg, "%i", (*retMsgSize));
                strcat(retMsg, ",");
                char* temp = malloc(intLength(errorcode));
                sprintf(temp, "%i", errorcode);
                strcat(retMsg, temp);
                strcat(retMsg, ",w");
                free(temp);
                return retMsg;
        } else {
                //now we are working with the case where the write was successful, in which case the message will always be the same
                //we know how long it should be, so we can mallloc it directly
                (*retMsgSize) += 2; //account for the msgsize and error code
                char* retMsg = malloc(sizeof(char) * 5);
                retMsg[0] = '4'; //msgsize
                retMsg[1] = ',';
                retMsg[2] = '0';             //error code
                retMsg[3] = ',';
                retMsg[4] = 'w'; //the always there 'w'
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

        close(fd);

        int found = findNode(fd);
        //let's first see if the file exists
        if (found != 1) { //the node wasn't found
                //let's set the returnMsgSize to 0 in case this isnt the first return
                (*retMsgSize) = 0;

                //first, we know that a return message will always have two commas in it and a c. Let's add 2 to the return counter
                (*retMsgSize) += (sizeof(char) * 3); //now we don't have to worry about the commas being counted anymore

                //lets get the return error
                int retErrorCode = errno; //set the error code to the errno value
                (*retMsgSize) += intLength(retErrorCode); //increment the retMsgSize counter by the length of the error

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc((*retMsgSize) + intLength((*retMsgSize))); //the return message is retMsgSize long, plus the length of retMsgSize
                //the first thing in the message is the retMsgSize, so let's add that
                //however, retMsgSize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", (*retMsgSize));
                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                strcat(retMsg, ",");
                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                char* tempErrorCode = malloc(sizeof(int) * intLength(retErrorCode) + sizeof(char)); //don't forget to add room for a null byte
                sprintf(tempErrorCode, "%i", retErrorCode);
                //now we can concat that
                strcat(retMsg, tempErrorCode);
                //now we have to add a comma
                strcat(retMsg, ",c");

                //time to be a responsible programmer and clean up what we malloced
                // free(retNegFd);
                free(tempErrorCode);
                return retMsg;
        } else {
                //okay, now it worked so the first thing we should do is to update the linked list
                removeNode(fd); //removes node

                //let's set the returnMsgSize to 0 in case this isnt the first return
                (*retMsgSize) = 0;

                //now we have to make the return message.
                //first, we know that a return message will always have two commas in it and a c. Let's add 2 to the return counter
                (*retMsgSize) += (sizeof(char) * 3); //now we don't have to worry about the commas being counted anymore

                //lets set the return error, which will be 0 to indicate a success
                int retErrorCode = 0;
                (*retMsgSize) += (sizeof(char)); //increment the retMsgSize counter

                //now we have the two components of the return message, let's create the final return message
                char* retMsg = malloc((sizeof(char) * (*retMsgSize)) + (sizeof(char) * intLength((*retMsgSize)))); //the return message is retMsgSize long, plus the length of retMsgSize
                //the first thing in the message is the retMsgSize, so let's add that
                //however, retMsgSize is an int right now and we need it as a char*, so let's cast it
                sprintf(retMsg, "%i", (*retMsgSize));
                //now retMsg starts with the length of the following string
                //time to add a ',' to seperate this
                strcat(retMsg, ",");
                //now we have to add the error code
                //so things get a little hairy here so hold on. We need to cast the error code from an int to a char*
                //however, we can't do it like we did a few lines before as snprintf overwrite the buffer. We need to make a new string to temperarily hold it
                char* tempErrorCode = malloc(sizeof(int) * intLength(retErrorCode) + sizeof(char)); //don't forget to add room for a null byte
                sprintf(tempErrorCode, "%i", retErrorCode);
                //now we can concat that
                strcat(retMsg, tempErrorCode);
                //now we have to add a comma
                strcat(retMsg, ",c");

                //time to be a responsible programmer and clean up what we malloced
                // free(retNegFd);
                free(tempErrorCode);
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

                //IF ITS NETINIT
                if(total_Size == MIN_BUFF_SIZE) {
                        char c = init_buffer[4];
                        init_buffer[4] = '\0';
                        if(strcmp(init_buffer, "chk?") == 0) { //Then its netinit
                                int s;
                                s = write(*((int*) sock),"good",5);
                                if (s < 0)
                                        threadError("ERROR writing to socket",__LINE__,errno);
                                free(init_buffer);
                                close(*((int*)sock));
                                free(sock);
                                pthread_exit(0);
                        }
                        init_buffer[4] = c;
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
        if(returningMsg == NULL) {
                free(returningMsg);
                close(*((int*)sock));
                free(sock);
                pthread_exit(0);
        }

        //Write the message back to the client
        status = write(*((int*) sock),returningMsg,retMsgSize);
        if (status < 0) {
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
