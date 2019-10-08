#ifndef INCLUDE_INTERRUPT_H_
#define INCLUDE_INTERRUPT_H_

#include "exception.h"

#define INT1_SR   0xbfd01058
#define INT1_EN   0xbfd0105c
#define INT1_CLR  0xbfd01064
#define INT1_POL  0xbfd01068
#define INT1_EDGE 0xbfd0106c

void interrupt_helper();
void int1_enable(int num);
void int1_init();
static void irq_ip3_helper();
static void irq_mac();
static void irq_timer();
void set_timer();
void increase_timer();

#endif