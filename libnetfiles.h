#include <unistd.h>

#ifndef _LIBNETFILES_H_
#define _LIBNETFILES_H_

#define PORTNUM 10897
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define MIN_BUFF_SIZE 5

extern int netserverinit(char * hostname);

extern int netopen(const char *pathname, int flags);

extern ssize_t netread(int fildes, void * buf, size_t nbyte);

extern ssize_t netwrite(int fildes, const void *buf, size_t nbyte);

extern int netclose(int fd);

#endif /* _LIBNETFILES_H_ */
