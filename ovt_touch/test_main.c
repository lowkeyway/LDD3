#include <stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/mman.h>


#define CHAR_DEVICE "/dev/OVT_TOUCH_CHAR0"

int main()
{
  int fd = 0;
  printf("Hello World!\n");
  fd = open(CHAR_DEVICE, O_RDWR);

  printf("fd = %d!\n", fd);

  close(fd);
  return 0;
}
