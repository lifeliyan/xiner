#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "dlp.h"
#include <sys/ioctl.h>
#include <string.h>

/*
void dlp_seq(char *num);
void dlp_if(char *num);
void dlp_resolution(char* num);
void dlp_channel(char *num);
*/
int fd = 0;

int main(int argc, const char *argv[])
{
	
	int i =1;
	fd =open("/dev/dlp",O_RDWR);
	if(fd == -1)
	{
		perror("open");
		exit(0);
	}

	if(argc < 2)
	{
		printf("Usage:argc of num error!!\n");
		printf("input: ./dlp name value\n");
	}
	if(!strcmp(argv[1],"N"))
	{
		ioctl(fd,DLP_NORMAL);
		printf("info:DLP_NORMAL\n");
	}else if(!strcmp(argv[1],"H"))
	{
		ioctl(fd,DLP_HORIZONTAL);
		printf("info:DLP_HORIZONTAL\n");
	}else if(!strcmp(argv[1],"V"))
	{
		ioctl(fd,DLP_VERTICAL);
		printf("info:DLP_VERTICAL\n");
	}else if(!strcmp(argv[1],"SEQ"))
	{
		printf("info:DLP_SEQ\n");
			ioctl(fd,DLP_SEQ,atoi(argv[2]));
		}else if(!strcmp(argv[1],"IF"))
		{
			printf("info:DLP_IF\n");
			ioctl(fd,DLP_IF,atoi(argv[2]));
		}else if(!strcmp(argv[1],"RES"))
		{
			printf("info:DLP_RESOLUTION\n");
			ioctl(fd,DLP_RESOLUTION,atoi(argv[2]));
		}else if(!strcmp(argv[1],"CHA"))
		{
			printf("info:DLP_CHANNEL\n");
			ioctl(fd,DLP_CHANNEL,atoi(argv[2]));
		}else
			{
			printf("input error\n");
		}

			close(fd);
		return 0;
	};





