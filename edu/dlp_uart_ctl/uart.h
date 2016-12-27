#ifndef UART_H
#define UART_H

int open_terminal(char* devname); // 0 : OK, -1 : ERROR

//how many data bits, how many stop bits,
// 'n' or 'N' : no parity
// 'o' or 'O' : odd parity
// 'e' or 'E' : even parity
void set_terminal(int fd, int rate, int data_bits, char parity, int stop_bits);
void reset_terminal(int fd);
void close_terminal(int fd);

void send_bytes(int fd, unsigned char* buf, int size); // send bytes to stm32
void recv_bytes(int fd, unsigned char* buf, int size); // recv bytes from stm32

#endif // UART_H
