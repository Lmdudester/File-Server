#include "libnetfiles.h"

int main(int argc, char *argv[]) {
    if(argc < 3){
      printf("Too few args\n");
      return -1;
    }

    if(netserverinit(argv[1], TRANS) == 0){
      printf("Host Does Exist.\n");
    } else {
      printf("Error, bad host");
    }

    //Write to the file
    int fd = netopen(argv[2],O_WRONLY);

    printf("Returned Open: %d ", fd);
    printf("Errno: %s - %d\n\n", strerror(errno), errno);

    char buff[15];
    buff[0] = '\0';
    strcpy(buff,"Hello my dude!");

    printf("Returned Write: %zd ", netwrite(fd, buff, 14));
    printf("Errno: %s - %d\n\n", strerror(errno), errno);

    printf("Returned Close: %d ", netclose(fd));
    printf("Errno: %s - %d\n\n", strerror(errno), errno);

    //Now read it back
    fd = netopen(argv[2],O_RDONLY);
    bzero(buff,15);

    //Open given File
    printf("Returned Open: %d ", fd);
    printf("Errno: %s - %d\n\n", strerror(errno), errno);

    printf("Returned Read: %zd ", netread(fd, buff, 5));
    printf("Errno: %s - %d\n\n", strerror(errno), errno);
    printf("First 5: %s\n", buff);
    bzero(buff,15);

    printf("Returned Read: %zd ", netread(fd, buff, 4));
    printf("Errno: %s - %d\n\n", strerror(errno), errno);
    printf("First 5: %s\n", buff);
    bzero(buff,15);

    printf("Returned Read: %zd ", netread(fd, buff, 5));
    printf("Errno: %s - %d\n\n", strerror(errno), errno);
    printf("First 5: %s\n", buff);
    bzero(buff,15);

    printf("Returned Close: %d ", netclose(fd));
    printf("Errno: %s - %d\n\n", strerror(errno), errno);

    return 0;
}
