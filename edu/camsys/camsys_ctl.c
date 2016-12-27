#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define CAMSYS_IOC_MAGIC  'M'
#define CAMSYS_IOC_MAXNR  14
 
#define CAMSYS_VERCHK            _IOR(CAMSYS_IOC_MAGIC,  0, camsys_version_t)
#define CAMSYS_I2CRD             _IOWR(CAMSYS_IOC_MAGIC,  1, camsys_i2c_info_t)
#define CAMSYS_I2CWR             _IOW(CAMSYS_IOC_MAGIC,  2, camsys_i2c_info_t)
#define CAMSYS_SYSCTRL           _IOW(CAMSYS_IOC_MAGIC,  3, camsys_sysctrl_t) 
#define CAMSYS_REGRD             _IOWR(CAMSYS_IOC_MAGIC,  4, camsys_reginfo_t)
#define CAMSYS_REGWR             _IOW(CAMSYS_IOC_MAGIC,  5, camsys_reginfo_t)
#define CAMSYS_REGISTER_DEVIO    _IOW(CAMSYS_IOC_MAGIC,  6, camsys_devio_name_t)
#define CAMSYS_DEREGISTER_DEVIO  _IOW(CAMSYS_IOC_MAGIC,  7, unsigned int)
#define CAMSYS_IRQCONNECT        _IOW(CAMSYS_IOC_MAGIC,  8, camsys_irqcnnt_t)
#define CAMSYS_IRQWAIT           _IOR(CAMSYS_IOC_MAGIC,  9, camsys_irqsta_t)
#define CAMSYS_IRQDISCONNECT     _IOW(CAMSYS_IOC_MAGIC,   10, camsys_irqcnnt_t)
#define CAMSYS_QUREYMEM          _IOR(CAMSYS_IOC_MAGIC,  11, camsys_querymem_t)
#define CAMSYS_QUREYIOMMU        _IOW(CAMSYS_IOC_MAGIC,  12, int)


typedef enum camsys_sysctrl_ops_e {
  
      CamSys_Vdd_Start_Tag,
     CamSys_Avdd,
     CamSys_Dovdd,
     CamSys_Dvdd,
     CamSys_Afvdd,
     CamSys_Vdd_End_Tag,
 
     CamSys_Gpio_Start_Tag,
     CamSys_PwrDn,
     CamSys_Rst,
     CamSys_AfPwr,
     CamSys_AfPwrDn,
     CamSys_PwrEn,
     CamSys_Gpio_End_Tag,
 
     CamSys_Clk_Start_Tag,
     CamSys_ClkIn,
     CamSys_Clk_End_Tag,
 
     CamSys_Phy_Start_Tag,
     CamSys_Phy,
     CamSys_Phy_End_Tag,
     CamSys_Flash_Trigger_Start_Tag,
     CamSys_Flash_Trigger,
     CamSys_Flash_Trigger_End_Tag,
     CamSys_IOMMU
 
 } camsys_sysctrl_ops_t;

 typedef struct camsys_sysctrl_s {
     unsigned int              dev_mask;
     camsys_sysctrl_ops_t      ops;
     unsigned int              on;
 
     unsigned int              rev[20];
 } camsys_sysctrl_t;



int main()
{
	int fd = -1;
	int err;
	fd = open("/dev/camsys_marvin",O_RDWR);
	if(fd < 0)
		printf("open device fail\n");
	camsys_sysctrl_t sysctl;	
/*
 	printf("power en...\n");
     	usleep(1000);
//CAMSYS_DEVID_SENSOR_1B  CAMSYS_DEVID_SENSOR_2
     	sysctl.dev_mask = CAMSYS_DEVID_SENSOR_1B;
     	sysctl.ops = CamSys_PwrEn;
     	sysctl.on = 1;
     	err = ioctl(camsys_fd, CAMSYS_SYSCTRL, &sysctl);
     	if (err<0) {
         printf("CamSys_PwrDn on failed\n");
*/
//CAMSYS_DEVID_SENSOR_1A CAMSYS_DEVID_SENSOR_1B CAMSYS_DEVID_SENSOR_
	 usleep(1000);
    	sysctl.dev_mask = 0x02000000;
     	sysctl.ops = CamSys_PwrDn;//CamSys_Rst;
     	sysctl.on = 1;//0;
     	err = ioctl(fd, CAMSYS_SYSCTRL, &sysctl);
     	if (err<0) {
         printf("CamSys_PwrDn on failed\n");
}
	return 0;

}
