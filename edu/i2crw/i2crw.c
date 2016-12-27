#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/ioctl.h>
#include<string.h>
#include<unistd.h>
//#include <media/camsys_head.h>
//#include "/home/liyan/work/android/edu/kernel/include/media/camsys_head.h"
#define DEVICE_NAME  "/dev/camsys_marvin"

#define CAMSYS_IOC_MAGIC  'M'
#define CAMSYS_I2CRD             _IOWR(CAMSYS_IOC_MAGIC,  1, camsys_i2c_info_t)
#define CAMSYS_I2CWR             _IOW(CAMSYS_IOC_MAGIC,  2, camsys_i2c_info_t)

typedef struct camsys_i2c_info_s {
      unsigned char     bus_num;
      unsigned short    slave_addr;
      unsigned int      reg_addr;       //i2c device register address
      unsigned int      reg_size;       //register address size
      unsigned int      val;
      unsigned int      val_size;       //register value size
      unsigned int      i2cbuf_directly;
      unsigned int      i2cbuf_bytes;
      unsigned int      speed;          //100000 == 100KHz
  } camsys_i2c_info_t;

camsys_i2c_info_t i2cinfo;
int main(int argc,char* argv[])
{

	int camsys_fd;
	if((camsys_fd = open(DEVICE_NAME,O_RDWR)) < 0){
		perror("Unable to open i2c control file");
		exit(1);	
	}else{
		printf("Open %s success\n",DEVICE_NAME);
	}
	if(argc >= 3 && !strcmp(argv[1],"r")){
		i2cinfo.bus_num = 3;
		i2cinfo.slave_addr = 0x78;
		i2cinfo.reg_addr = strtol(argv[2],NULL,0);
	        printf("i2cinfo.reg_addr: 0x%x\n",i2cinfo.reg_addr);
		i2cinfo.reg_size = 2;
		i2cinfo.val = 0;
		i2cinfo.val_size = 2;
		i2cinfo.i2cbuf_directly=0;
		//i2cinfo.i2cbuf_bytes;
		i2cinfo.speed =100000;
		
		if(ioctl(camsys_fd, CAMSYS_I2CRD, &i2cinfo) < 0){
			perror("Unable read data");
			return -1;
		}else{
			printf("addr:ox%x  val: 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
		}
		
	}else if(argc >= 4 && !strcmp(argv[1],"w")){
		i2cinfo.bus_num = 3;
                  i2cinfo.slave_addr = 0x78;
                  i2cinfo.reg_addr = strtol(argv[2],NULL,0);
		  printf("i2cinfo.reg_addr: 0x%x\n",i2cinfo.reg_addr);
                  i2cinfo.reg_size = 2;
                  i2cinfo.val = strtol(argv[3],NULL,0);
		if( ioctl(camsys_fd, CAMSYS_I2CWR, &i2cinfo) < 0){
			perror("unable write data");
			return -1;
		}else{
			printf("addr: 0x%x, val: 0x%x\n",i2cinfo.reg_addr,i2cinfo.val);
		}	
	
	}else{
		printf("argc  to small\n");
	}	
	//close(camsys_fd);

	return 0;
}
