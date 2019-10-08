#ifndef INCLUDE_COMMON_H_
#define INCLUDE_COMMON_H_

#include "type.h"

#define PORT_URT0   0xbfe40000
#define PORT_URT1   0xbfe44000
#define PORT_URT2   0xbfe48000
#define PORT_URT3   0xbfec0000
#define PORT_URT4   0xbfec4000
#define PORT_URT5   0xbfec5000
#define PORT_URT6   0xbfec6000
#define PORT_URT7   0xbfec7000
#define PORT_URT8   0xbfec8000
#define PORT_URT9   0xbfec9000
#define PORT_URT10  0xbfeca000
#define PORT_URT11  0xbfecb000

#define REG_DAT     0x00
#define REG_IER     0x01
#define REG_IIR     0x02
#define REG_FCR     0x02
#define REG_LCR     0x03
#define REG_MCR     0x04
#define REG_LSR     0x05
#define REG_MSR     0x06
#define REG_CR      0x08
#define REG_MR      0x09

#define FORE_COLOR_BLACK    30
#define FORE_COLOR_RED      31
#define FORE_COLOR_GREEN    32
#define FORE_COLOR_YELLOW   33
#define FORE_COLOR_BLUE     34
#define FORE_COLOR_MAGENTA  35
#define FORE_COLOR_CYAN     36
#define FORE_COLOR_WHITE    37
#define FORE_COLOR_RESET    0

#define BACK_COLOR_BLACK    40
#define BACK_COLOR_RED      41
#define BACK_COLOR_GREEN    42
#define BACK_COLOR_YELLOW   43
#define BACK_COLOR_BLUE     44
#define BACK_COLOR_MAGENTA  45
#define BACK_COLOR_CYAN     46
#define BACK_COLOR_WHITE    47
#define BACK_COLOR_RESET    0

// enter a char into serial port
// use PMON printch function
void port_write_ch(char ch);

// enter a message into seraial port
// use PMON printstr function
void port_write(char *buf);

void read_uart_ch(void);

void *memcopy(void *s1, const void *s2, int n);
unsigned int write_block(unsigned long int write_addr, unsigned char *write_buffer);
int isQEMU();
// n Bytes, n & base should be sector aligned
int sdwrite(unsigned char *buf, unsigned int base, int n);
int sdread(unsigned char *buf, unsigned int base, int n);

#endif
