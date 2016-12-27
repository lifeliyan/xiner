#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include "uart.h"

#define DEVNAME "/dev/ttyS3"

void* recv_fun(void* fp);
void* send_fun(void* fp);

/*
 *  -------------------------------------------------------------------------
 *  | name  |  sop | len | addrh | addrl | dev type | cmd/para | data | fcs |
 *  -------------------------------------------------------------------------
 *  | bytes |  1   |  1  |   1   |   1   |    1     |    1     | 0~n  |  1  |
 *  -------------------------------------------------------------------------
 *
 *  sop : 0xFE
 *  len : dev type + cmd/para + data = len
 *  fcs : 
 */

// packet data buffer length
#define PACKET_DATA_LEN 1024
struct Packet {
	unsigned char sop;
	unsigned char len; //len = dev type + cmd/para + data
	unsigned char addrh;
	unsigned char addrl;
	unsigned char dev_type;
	unsigned char cmd;
	unsigned char data[PACKET_DATA_LEN];
	int           data_buf_len; // how many data buffer allocated
	int           data_len; // how many data in data buffer
	unsigned char fcs;
};

// initialize a packet
void packet_init(struct Packet* packet);

// receive a packet from a given file descriptor
void packet_recv(int fd, struct Packet* packet);

// send a packet to a given file descriptor
// 0 success
int packet_send(int fd, struct Packet const * packet);

// print out a packet's content
void packet_print(struct Packet const * packet);

// check if a packet is valid
// true valid
bool packet_valid(struct Packet const * packet);

// return xor value of data
unsigned char fcs(unsigned char* data, int size)
{
	unsigned char result = 0;
	int i;
	for(i = 0; i < size; i++) {
		result = result ^ *(data + i);
	}

	return result;
}

//int main(void)
int main(int argc, char *argv[])
{

        char buf[9] = {0};
        int str_size = 0;

	str_size = strlen(argv[1]);
	printf("str_size is %d, DEVICE is %s \n",str_size, DEVNAME);

	if( str_size >= 8 )
	{
		perror("The lenth of char array is out of range \n");
		printf("The lenth of char array is out of range \n");
		return -1;
	}

	strcpy(buf,argv[1]);
	printf("buf is %s \n",buf);


	int fd;
	fd = open_terminal(DEVNAME);
	if(-1 == fd) {
		perror("open terminal device");
		return -1;
	}

	// 57600, 8n1
//	set_terminal(fd, 2400, 8, 'n', 1);
//	set_terminal(fd, 9600, 8, 'n', 1);
	set_terminal(fd, 19200, 8, 'n', 1);
//	set_terminal(fd, 115200, 8, 'n', 1);

  // send bytes to stm32
	if(-1 == write(fd, buf, str_size)) 
        {
		perror("send bytes to terminal");
	}
  //

//zhaojun 
	perror("set string  finished ") ;
	printf("set string %s finished ", buf) ;

	return 0;
}
void packet_init(struct Packet* packet)
{
	packet->sop = 0;
	packet->len = 0; //len = dev type + cmd/para + data
	packet->addrh = 0;
	packet->addrl = 0;
	packet->dev_type = -1;
	packet->cmd = 0;
	packet->data_buf_len = PACKET_DATA_LEN;
	packet->data_len = 0;
	//packet->data = NULL; // now not dynamic alloc memory, don't need this
	packet->fcs = 0;
};

#define SOP_STATE      0x00
#define CMD_STATE1     0x01
#define CMD_STATE2     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05
#define MT_UART_SOF    0xFE

// receive a packet from a given file descriptor
void packet_recv(int fd, struct Packet* packet)
{

  int n;
  unsigned char  ch;
  unsigned char buffer[1024];

  // status machine initial statue
  unsigned char state = SOP_STATE;


  int read_index = 0;
  int temp_data_len = 0;

  memset(buffer, '\0', 1024);
  //n = recv_bytes(fd, buffer, 1024);
  //n = read(fd, buffer, 20);
  //n = read(fd, buffer, 1024);


  while (1)
  {
    //HalUARTRead (port, &ch, 1);
	// read sop byte
//	ch = buffer[read_index++];

  printf("befor read %s %d\n", __func__, __LINE__);
	read(fd, &ch, 1);
  printf("after read %s %d, ch is 0x%x\n", __func__, __LINE__, ch);

    switch (state)
    {
      case SOP_STATE:
		  printf("SOP_STATE\n");
        if (ch == MT_UART_SOF)
          state = LEN_STATE;
		packet->sop = ch;
        break;

      case LEN_STATE:
		  printf("LEN_STATE\n");
        //LEN_Token = ch;
		packet->len = ch;

		state = CMD_STATE1;
        break;

      case CMD_STATE1: // addrh
		  printf("CMD_STATE1\n");
		packet->addrh = ch;
        state = CMD_STATE2;
        break;

      case CMD_STATE2: // addrl
		  printf("CMD_STATE2\n");
		packet->addrl = ch;
        /* If there is no data, skip to FCS state */
        if (0 != packet->len)
        {
          state = DATA_STATE;
        }
        else
        {
          state = FCS_STATE;
        }
        break;

      case DATA_STATE:
		  printf("DATA_STATE\n");

		if(packet->len >= 1) {
			packet->dev_type = ch;
	//		ch = buffer[read_index++];
	read(fd, &ch, 1);
			temp_data_len++;
		}
		if(packet->len >= 2) {
			packet->cmd = ch;
	//		ch = buffer[read_index++];
	read(fd, &ch, 1);
			temp_data_len++;
		}
		if(packet->len >= 3) {
			int i;
			for(i = 0; i < packet->len - 2; i++) {
				read(fd, packet->data + i, 1);
			}
			//memcpy(packet->data, buffer+read_index, packet->len - 2);
			read_index += packet->len - 2;
			temp_data_len += packet->len - 2;
		}


        /* If number of bytes read is equal to data length, time to move on to FCS */
        if (temp_data_len == packet->len ) {
            state = FCS_STATE;
			temp_data_len = 0;
		} else {
			printf("status machine error at %s %d\n", __func__, __LINE__);
		}

        break;

      case FCS_STATE:
		  printf("FCS_STATE\n");

		packet->fcs = ch;

		// now don't check fcs byte
		// need code here.

        /* Reset the state, send or discard the buffers at this point */
        state = SOP_STATE;

		return;
        break;

      default:
       break;
    }
  }

}

int packet_send(int fd, struct Packet const * packet)
{
	char buf[1024];
	int send_data_len = 0;

	buf[0] = packet->sop; // sop
	buf[1] = packet->len; // data length
	buf[2] = packet->addrh; // high address
	buf[3] = packet->addrl; // low address

	send_data_len += 4;

	if(packet->len >= 1) { // device type
		buf[send_data_len++] = packet->dev_type;
	}
	if(packet->len >= 2) { // cmd
		buf[send_data_len++] = packet->cmd;
	}

	if(packet->len >= 3) { // data
		int i;
		for(i = 0; i < packet->len - 2; i++) {
			buf[send_data_len++] = packet->data[i];
		}
	}

	buf[send_data_len++] = packet->fcs; // fcs (xor sum)

	if(0 != write(fd, buf, send_data_len)) {
		perror("packet_send write error");
		return -1;
	}

	return 0;
}

void packet_print(struct Packet const * packet)
{
	int i;
	printf("packet sop          : 0x%x\n", packet->sop);
	printf("packet len          : 0x%x\n", packet->len);
	printf("packet addrh        : 0x%x\n", packet->addrh);
	printf("packet addrl        : 0x%x\n", packet->addrl);
	printf("packet dev_type     : 0x%x\n", packet->dev_type);
	printf("packet cmd          : 0x%x\n", packet->cmd);
	printf("packet data_buf_len : 0x%x\n", packet->data_buf_len);
	printf("packet data_len     : 0x%x\n", packet->data_len);
	printf("packet data: ");
	for(i = 0; i < packet->data_len; i++) {
		printf("0x%0x ", packet->data[i]);
	}
	printf("\n");
	printf("packet fcs          : 0x%x\n", packet->fcs);
}

bool packet_valid(struct Packet const * packet)
{

	char buf[1024];
	if(packet->len >= 1) {
		buf[0] = packet->dev_type;
	}
	if(packet->len >= 2) {
		buf[1] = packet->cmd;
	}
	if(packet->len >= 3) {
		memcpy(buf+2, packet->data, packet->len - 2);
	}
	// check if have right packet head
	if(0xFE != packet->sop) {
		printf("Packet with wrong sop\n");
		return false;
	}

	// check if check sum is ok
	if(packet->fcs != fcs(buf, packet->len)) {
		return false;
	}

	return true;
}


void* recv_fun(void* fp)
{
	int n;
	int i;
	unsigned char buf[1024];

	int fd = *(int*)fp;
	struct Packet packet;

	while(1) {
		/*
		packet_recv(fd, &packet);
		if(packet_valid(&packet)) {
			packet_print(&packet);
		} else {
			printf("Invalid packet received\n");
		}
		*/
		memset(buf, '\0', 1024);
		printf("befor read\n");
		n = read(fd, buf, 20);
		printf("after read\n");
		if(n < 0) {
			perror("read terminal");
		}
		for(i = 0; i < n; i++) {
			printf("0x%x ", buf[i]);
		}
		printf("\n");
	}
}

void* send_fun(void* fp)
{
	char buf[1024];
	int fd = *(int*)fp;
	int n;

	

	while(1) {
		printf("Send thread\n");
		buf[0] = 0xFE;
		buf[1] = 0x00;
		buf[2] = 0x56;
		buf[3] = 0x54;
		buf[4] = 0x01;
		buf[5] = 0x01;
		buf[6] = 0x02;

		n = write(fd, buf, 7);
		if(n < 0) {
			perror("write terminal");
		}
		sleep(1);

		buf[0] = 0xFE;
		buf[1] = 0x00;
		buf[2] = 0x56;
		buf[3] = 0x54;
		buf[4] = 0x01;
		buf[5] = 0x02;
		buf[6] = 0x01;
		n = write(fd, buf, 7);
		if(n < 0) {
			perror("write terminal");
		}
		sleep(1);
		//buf[0] = 0xFE;
		/*
		memset(buf, '\0', 1024);
		//scanf("%s", buf);
		fgets(buf, 1024, stdin);
		for(n = 0; n < 1024; n++) {
			printf("0x%d ", buf[n]);
		}
//		strcat(buf, "\n");
*/
		n = write(fd, buf, strlen(buf));
		if(n < 0) {
			perror("write terminal");
		}
	}
}
