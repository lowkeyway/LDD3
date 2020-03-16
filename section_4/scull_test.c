#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define DEV_PATH	"/dev/scull"
#define BUFSIZE		8
#define COUNTS		100

pthread_mutex_t mutex;
int count = 0;

static void *read_routine(void *arg)
{
	printf("%s %d\n", __FUNCTION__, __LINE__);

	int fd;
	fd = open(DEV_PATH, O_RDWR);
	if(fd < 0)
	{
		printf("%s, %d, Open %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
		pthread_exit(NULL);
	}

	char buffer[BUFSIZE] = {0};
	memset(buffer, '\0', sizeof(buffer));
	int count = COUNTS;
	int i = 0;

	do
	{
		if(read(fd, buffer, sizeof(buffer)) != sizeof(buffer))
		{
			printf("%s, %d. read buffer %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
			sleep(1);
			continue;
		}

		pthread_mutex_lock(&mutex);
		printf("<---- \t Read buffer[%d]: %s\n", i++, buffer);
		pthread_mutex_unlock(&mutex);


		memset(buffer, '\0', sizeof(buffer));
		sleep(1);
	}while(count--);

	close(fd);
}

static void *write_routine(void *arg)
{
	printf("%s %d\n", __FUNCTION__, __LINE__);
	
	int i = 0, times = 0;
	int fd = -1;
	char buffer[BUFSIZE] = {0};


	printf("buffer=[%s]\n", buffer);

	fd = open(DEV_PATH, O_RDWR);
	if(fd < 0)
	{
		printf("%s, %d. Open %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
		pthread_exit(NULL);
	}

	i = 0;
	count = COUNTS;

	do
	{
		for(i = 0; i < sizeof(buffer) - 1; i++)
		{
			buffer[i] = 'a' + times%26;
		}
		buffer[BUFSIZE - 1] = '\0';

		if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer))
		{
			printf("%s, %d. Write buffer %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
			sleep(1);
			continue;
		}

		pthread_mutex_lock(&mutex);
		printf("----> \t write buffer[%d]: %s\n", times++, buffer);
		pthread_mutex_unlock(&mutex);

		sleep(1);
	}while(count--);

	close(fd);
}

int main()
{
	int fd = -1, data = -1;
	int i = 0;

	printf("%s %d\n", __FUNCTION__, __LINE__);

	pthread_t p_read;
	pthread_t p_write;

	if(access(DEV_PATH, 0))
	{
		printf("%s not exits\n", DEV_PATH);
		return -1;
	}

	pthread_mutex_init(&mutex, NULL);

	if(pthread_create(&p_read, NULL, read_routine, NULL))
	{
		printf("%s, %d, read thread create fail!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(pthread_create(&p_write, NULL, write_routine, NULL))
	{
		printf("%s, %d, write thread create fail!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	pthread_join(p_read, NULL);
	pthread_join(p_write, NULL);
		
#if 0
	fd = open(DEV_PATH, O_RDWR);
	if(fd < 0)
	{
		printf("%s %d, Open %s fail! fd=%d\n", __FUNCTION__, __LINE__, DEV_PATH, fd);
		return fd;
	}

	if(read(fd, &data, sizeof(data)) < 0)
	{
		printf("%s %d, read %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
		return -1;
	}

	printf("Read Init Data is %d\n", data);
	
	for(i = 0; i < 10; i++)
	{
		data+=100;
		if(write(fd, &data, sizeof(data)) != sizeof(data))
		{
			printf("%s %d, write %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
			return -1;

		}

		if(read(fd, &data, sizeof(data)) < 0)
		{
			printf("%s %d, read %s fail!\n", __FUNCTION__, __LINE__, DEV_PATH);
			return -1;
		}

		printf("Read Data %dth is %d\n", i, data);
	}
#endif
	return 0;
}
