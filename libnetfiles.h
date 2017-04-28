#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _LIBNETFILES_H_
#define _LIBNETFILES_H_

#define PORTNUM 10897
#define UNRST 0
#define EXCLU 1
#define TRANS 2
#define MIN_BUFF_SIZE 5

extern int netserverinit(char * hostname, int filemode);

extern int netopen(const char *pathname, int flags);

extern ssize_t netread(int fildes, void * buf, size_t nbyte);

extern ssize_t netwrite(int fildes, const void *buf, size_t nbyte);

extern int netclose(int fd);

#endif /* _LIBNETFILES_H_ */
