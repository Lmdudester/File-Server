#include "libnetfiles.h"

int main(int argc, char *argv[]) {
  if(argc != 2){
    printf("Use: ./myCli <server>\n");
    return -1;
  }

  if(netserverinit(argv[1], TRANS) == 0){
    printf("Host Does Exist.\n");
  } else {
    printf("Error, bad host. Exiting...\n");
    return -1;
  }

  int fd1 = netopen("./tests/test1.txt", O_WRONLY);
  int fd2 = netopen("./tests/test1.txt", O_RDONLY);
  int fd3 = netopen("./tests/test1.txt", O_WRONLY);
  int fd4 = netopen("./tests/test1.txt", O_RDONLY);

  if(fd1 != -1 && fd2 == -1 && fd3 == -1 && fd4 == -1)
    printf("Success...\n");

  fd1 = netclose(fd1);
  fd2 = netclose(fd2);
  fd3 = netclose(fd3);
  fd4 = netclose(fd4);

  if(fd1 == 0 && fd2 == -1 && fd3 == -1 && fd4 == -1)
    printf("Success...\n");

  return 0;
}
