#ifndef INCLUDE_MAC_H_
#define INCLUDE_MAC_H_

#include "type.h"
#include "queue.h"
#include "mac_enum.h"

#define GMAC_BASE_ADDR (0xbfe10000)
#define DMA_BASE_ADDR  (0xbfe11000)

#define PSIZE (256)
#define PNUM  (64)

extern queue_t recv_block_queue;
extern uint32_t recv_flag[PNUM];
extern uint32_t ch_flag;

typedef struct desc
{
    uint32_t des0;
    uint32_t des1;
    uint32_t des2;
    uint32_t des3;
} desc_t;

typedef struct mac
{
    uint32_t psize; // package size
    uint32_t pnum;  // package number
} mac_t;

uint32_t read_register(uint32_t base, uint32_t offset);
void reg_write_32(uint32_t addr, uint32_t data);
uint32_t reg_read_32(uint32_t addr);

void disenable_dma_int();
void enable_dma_int();

void do_init_mac();
void do_wait_recv_package();
void check_recv();

void do_net_send();
void do_net_recv();
void send_desc_init(mac_t *mac);
void recv_desc_init(mac_t *mac);

#define TDESC_LENGTH 64
#define TBUFFER_SIZE 1024
#define RDESC_LENGTH 64
#define RBUFFER_SIZE 2047

extern desc_t tdesc[TDESC_LENGTH];
extern uint8_t tbuffer[TDESC_LENGTH][TBUFFER_SIZE];
extern desc_t rdesc[RDESC_LENGTH];
extern uint8_t rbuffer[RDESC_LENGTH][RBUFFER_SIZE];

void set_buffer(uint8_t *tbuffer, uint32_t *data);
void empty_buffer(uint8_t *buffer);

void clear_dma_interrupt();
void mii_dul_force();
void dma_control_init(int32_t init_value);

#endif