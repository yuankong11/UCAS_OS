#include "mac.h"
#include "irq.h"
#include "debug.h"
#include "stdio.h"
#include "exception.h"
#include "irq.h"

uint32_t reg_read_32(uint32_t addr)
{
    return *((volatile uint32_t *)addr);
}

uint32_t read_register(uint32_t base, uint32_t offset)
{
    uint32_t addr = base + offset;
    uint32_t data;

    data = *((volatile uint32_t *)addr);
    return data;
}

void reg_write_32(uint32_t addr, uint32_t data)
{
    *((volatile uint32_t *)addr) = data;
}

static void gmac_get_mac_addr(uint8_t *mac_addr)
{
    uint32_t addr;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0Low);
    mac_addr[0] = (addr >> 0) & 0x000000FF;
    mac_addr[1] = (addr >> 8) & 0x000000FF;
    mac_addr[2] = (addr >> 16) & 0x000000FF;
    mac_addr[3] = (addr >> 24) & 0x000000FF;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0High);
    mac_addr[4] = (addr >> 0) & 0x000000FF;
    mac_addr[5] = (addr >> 8) & 0x000000FF;
}

void set_sram_ctr()
{
    *((volatile unsigned int *)0xbfd00420) = 0x8000;
}

static void mac_reset()
{
    reg_write_32(DMA_BASE_ADDR, 0x01);

    while ((reg_read_32(DMA_BASE_ADDR) & 0x01))
        ;
}

void disable_dma_int()
{
    reg_write_32(DMA_BASE_ADDR + 28, 0x0);
}

void enable_dma_int()
{
    reg_write_32(DMA_BASE_ADDR + 28, 0x10041);
}

void set_mac_addr()
{
    uint32_t data;
    uint8_t MacAddr[6] = {0x00, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};
    uint32_t MacHigh = 0x40, MacLow = 0x44;
    data = (MacAddr[5] << 8) | MacAddr[4];
    reg_write_32(GMAC_BASE_ADDR + MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
    reg_write_32(GMAC_BASE_ADDR + MacLow, data);
}

void do_init_mac()
{
    set_sram_ctr();
    mac_reset();
    int1_init();
    int1_enable(3);
    clear_dma_interrupt();
    enable_dma_int();
    set_mac_addr();

    // enable MAC_TX, MAC_RX
    reg_write_32(GMAC_BASE_ADDR + 0, reg_read_32(GMAC_BASE_ADDR + 0x0) | 0x4);
    reg_write_32(GMAC_BASE_ADDR + 0, reg_read_32(GMAC_BASE_ADDR + 0x0) | 0x8);
}

desc_t tdesc[TDESC_LENGTH];
uint8_t tbuffer[TDESC_LENGTH][TBUFFER_SIZE];
desc_t rdesc[RDESC_LENGTH];
uint8_t rbuffer[RDESC_LENGTH][RBUFFER_SIZE];

#define phy_addr(x) ((x) & 0x1fffffff)

void set_buffer(uint8_t *buffer, uint32_t *data)
{
    // todo: length
    if((uint32_t)buffer % 4 != 0)
        panic("Unaligned buffer address.");
    uint32_t *buffer_32 = (uint32_t *)buffer;
    uint32_t *data_32;
    int i, j;
    for(i = 0; i < 64; ++i)
    {
        data_32 = data;
        for(j = 0; j < 1024/4; ++j)
            *(buffer_32++) = *(data_32++);
    }
}

void do_net_send()
{
    int i;

    // !!!the two line below should be in the order
    reg_write_32(DMA_BASE_ADDR + 16, phy_addr((uint32_t)&tdesc));
    reg_write_32(DMA_BASE_ADDR + 24, reg_read_32(DMA_BASE_ADDR + 24) | 0x02202000);

    for(i = 0; i < 64; ++i)
        tdesc[i].des0 = 0x80000000;

    for(i = 0; i < 64; ++i)
        reg_write_32(DMA_BASE_ADDR + 4, 0x1);
}

void empty_buffer(uint8_t *buffer)
{
    // todo: length
    if((uint32_t)buffer % 4 != 0)
        panic("Unaligned buffer address.");
    uint32_t *buffer_32 = (uint32_t *)buffer;
    int i, j;
    for(i = 0; i < 64; ++i)
    for(j = 0; j < 1024/4; ++j)
        *(buffer_32++) = 0;
}

void do_net_recv()
{
    int i;

    // !!!the two line below should be in the order
    reg_write_32(DMA_BASE_ADDR + 12, phy_addr((uint32_t)&rdesc));
    reg_write_32(DMA_BASE_ADDR + 24, reg_read_32(DMA_BASE_ADDR + 24) | 0x02200002);

    for(i = 0; i < 64; ++i)
        rdesc[i].des0 = 0x80000000;

    for(i = 0; i < 64; ++i)
        reg_write_32(DMA_BASE_ADDR + 8, 0x1);
}

queue_t recv_block_queue;

void do_wait_recv_package()
{
    do_wait(current_running, &recv_block_queue, TASK_BLOCKED);
}

void check_recv()
{
    if(queue_is_empty(&recv_block_queue))
        return;
    if(!(rdesc[63].des0 >> 31))
        do_unblock_one(&recv_block_queue);
}

void send_desc_init(mac_t *mac)
{
    int i;
    for(i = 0; i < 63; ++i)
    {
        tdesc[i].des0 = 0x0;
        tdesc[i].des1 = 0x61000400;
        tdesc[i].des2 = phy_addr((uint32_t)&tbuffer[i]);
        tdesc[i].des3 = phy_addr((uint32_t)&tdesc[i+1]);
    }
    tdesc[63].des0 = 0x0;
    tdesc[63].des1 = 0x63000400;
    tdesc[63].des2 = phy_addr((uint32_t)&tbuffer[63]);
    tdesc[63].des3 = phy_addr((uint32_t)&tdesc[0]);
}

void recv_desc_init(mac_t *mac)
{
    int i;
    for(i = 0; i < 63; ++i)
    {
        rdesc[i].des0 = 0x0;
        rdesc[i].des1 = 0x010007ff;
        rdesc[i].des2 = phy_addr((uint32_t)&rbuffer[i]);
        rdesc[i].des3 = phy_addr((uint32_t)&rdesc[i+1]);
    }
    rdesc[63].des0 = 0x0;
    rdesc[63].des1 = 0x030007ff;
    rdesc[63].des2 = phy_addr((uint32_t)&rbuffer[63]);
    rdesc[63].des3 = phy_addr((uint32_t)&rdesc[0]);
}

void clear_dma_interrupt()
{
    uint32_t data;
    data = reg_read_32(DMA_BASE_ADDR + DmaStatus);
    reg_write_32(DMA_BASE_ADDR + DmaStatus, data);
}

void mii_dul_force()
{
    reg_write_32(DMA_BASE_ADDR, 0x0);
    uint32_t conf = 0xc800;

    // loopback, 100M
    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | (conf) | (1 << 8));
    // enable recieve all
    reg_write_32(GMAC_BASE_ADDR + 0x4, reg_read_32(GMAC_BASE_ADDR + 0x4) | 0x80000001);
}

void dma_control_init(int32_t init_value)
{
    reg_write_32(DMA_BASE_ADDR + DmaControl, init_value);
}