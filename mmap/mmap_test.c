#include <stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/mman.h>

#define MMAP_SIZE 4096

static dump_mmap(char* p, int len)
{
  int i;
  for(i = 0; i < len; i++)
  {
    if(0 == i%10) printf("\n");
    printf("%4d ", p[i]);
  }
  printf("\n");
}
 
int main()
{
    int fd;
    char *start;
    //char buf[100];
    char *buf;
    
    /*打开文件*/
    fd = open("/dev/memdev0",O_RDWR);
        
    buf = (char *)malloc(MMAP_SIZE);
    memset(buf, 0, MMAP_SIZE);
    printf("begain mmap");
    dump_mmap(buf, 100);
    start=mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    
    /* 读出数据 */
    //strcpy(buf,start);
    memcpy(buf, start, 100);
    dump_mmap(start, 100);
    sleep (1);
     
       
    munmap(start,MMAP_SIZE); /*解除映射*/
    free(buf);
    close(fd);  
    return 0;    
}
