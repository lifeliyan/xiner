#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include "uart.h"


static struct termios old_opt;
static struct termios new_opt;

int open_terminal(char* devname); // 0 : OK, -1 : ERROR
void set_terminal(int fd, int rate, int data_bits, char parity, int stop_bits);
void reset_terminal(int fd);
void close_terminal(int fd);

void send_bytes(int fd, unsigned char* buf, int size); // send bytes to stm32
void recv_bytes(int fd, unsigned char* buf, int size); // recv bytes from stm32


void send_bytes(int fd, unsigned char* buf, int size)
{
	// send bytes to stm32
	if(-1 == write(fd, buf, size)) {
		perror("send bytes to terminal");
	}
}

void recv_bytes(int fd, unsigned char* buf, int size)
{
	// recv bytes from stm32
	if(-1 == read(fd, buf, size)) {
		perror("recv bytes from terminal");
	}
}

int open_terminal(char* devname)
{
	int fd;
	//int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	fd = open(devname, O_RDWR | O_NOCTTY); // should use block IO

	return fd;
}

void reset_terminal(int fd)
{
	// restore the old configuration
	int ret;
	ret = tcsetattr(fd, TCSANOW, &old_opt);
	if(ret < 0) {
		printf("Can't set terminol\n");
	}
}

// how many data bits, how many stop bits,
// 'n' or 'N' : no parity
// 'o' or 'O' : odd parity
// 'e' or 'E' : even parity
void set_terminal(int fd, int rate, int data_bits, char parity, int stop_bits)
{
	int set_speed = 0;
	/* get current options
	*/
	tcgetattr(fd, &old_opt);
	new_opt = old_opt;

	/* set baud rate
	*/
	switch(rate) {
		case 0: set_speed = B0; break;
		case 50: set_speed = B50; break;
		case 75: set_speed = B75; break;
		case 110: set_speed = B110; break;
		case 134: set_speed = B134; break;
		case 150: set_speed = B150; break;
		case 200: set_speed = B200; break;
		case 300: set_speed = B300; break;
		case 600: set_speed = B600; break;
		case 1200: set_speed = B1200; break;
		case 1800: set_speed = B1800; break;
		case 2400: set_speed = B2400; break;
		case 4800: set_speed = B4800; break;
		case 9600: set_speed = B9600; break;
		case 19200: set_speed = B19200; break;
		case 38400: set_speed = B38400; break;
		case 57600: set_speed = B57600; break;
		//case 76800: set_speed = B76800; break;
		case 115200: set_speed = B115200; break;
		default:
					 printf("Unknow terminal speed!! set terminal rate to B0\n");
					 set_speed = B0;
					 break;
	}
	cfsetispeed(&new_opt, set_speed);
	cfsetospeed(&new_opt, set_speed);
	/*
	   cfsetispeed(&new_opt, B9600);
	   cfsetospeed(&new_opt, B9600);
	   */

	/* enable the receive and set local mode...
	*/
	new_opt.c_cflag |= (CLOCAL | CREAD);


	// disble software flow control
	new_opt.c_iflag &= ~(IXON | IXOFF | IXANY);
	/* 
	 * Disable hardware flow control 
	 */  
	new_opt.c_cflag &= ~CRTSCTS;


	// set parity
	switch(parity) {
		case 'n':
		case 'N': // no parity
			new_opt.c_cflag &= ~PARENB;

			// must have this line, or data bit will be removed as parity bit.
			new_opt.c_iflag &= ~ISTRIP; //don't strip input parity bit
			/*
			new_opt.c_iflag &= ~INPCK;
			new_opt.c_iflag &= ~ISTRIP; //don't strip input parity bit
			*/
			break;
		case 'o':
		case 'O': // odd parity
			new_opt.c_cflag |= (PARODD | PARENB);
			new_opt.c_iflag |= INPCK;
			new_opt.c_iflag |= ISTRIP; //strip input parity bit
			break;
		case 'e':
		case 'E': // even parity
			new_opt.c_cflag |= PARENB;
			new_opt.c_cflag &= ~PARODD;
			new_opt.c_iflag |= INPCK;
			new_opt.c_iflag |= ISTRIP; //strip input parity bit
			break;
		default:
			printf("Unknow parity, set to even parity\n");
			new_opt.c_cflag |= PARENB;
			new_opt.c_cflag &= ~PARODD;
			new_opt.c_iflag |= INPCK;
			new_opt.c_iflag |= ISTRIP; //strip input parity bit
	}


	// set stop bits
	if(1 == stop_bits) {
		new_opt.c_cflag &= ~CSTOPB; // one stop bit
	} else if(2 == stop_bits) {
		new_opt.c_cflag |= CSTOPB; // two stop bit
	} else {
		printf("Unknow terminal stop bit, set to one stop bit!!!\n");
		new_opt.c_cflag &= ~CSTOPB;
	}


	// set how many data bits
	new_opt.c_cflag &= ~CSIZE;
	if(8 == data_bits) {
		new_opt.c_cflag |= CS8;     // eight data bits
	} else if(7 == data_bits) {
		new_opt.c_cflag |= CS7;     // seven data bits
	} else if(6 == data_bits) {
		new_opt.c_cflag |= CS6;     // six data bits
	} else if(5 == data_bits) {
		new_opt.c_cflag |= CS5;
	} else {
		printf("Unknow terminal data bits, set to 8 data bits!!\n");
		new_opt.c_cflag |= CS8;
	}

	/* classical input
	*/

	//new_opt.c_lflag |= (ICANON | ECHO | ECHOE);

	/* raw input
	*/
	new_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* select raw output
	*/
	new_opt.c_oflag &= ~OPOST;

	/* set the new option of the port
	*/
	int ret;
	ret = tcsetattr(fd, TCSANOW, &new_opt);
	if(ret < 0) {
		printf("Can't set terminol\n");
	}

	tcflush(fd, TCIFLUSH);
}

void close_terminal(int fd)
{
	close(fd);
}
