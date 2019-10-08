#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "time.h"
#include "screen.h"
#include "mac.h"

void int1_enable(int num)
{
    uint32_t int1_en = *((uint32_t *)INT1_EN);
    int1_en = int1_en | (0x1 << num);
    *((uint32_t *)INT1_EN) = int1_en;
}

void int1_init()
{
    *((uint32_t *)INT1_CLR)  = 0xffffffff;
    *((uint32_t *)INT1_POL)  = 0xffffffff;
    *((uint32_t *)INT1_EDGE) = 0x0;
}

void interrupt_helper()
{
    // Leve3 exception Handler.
    // read CP0 register to analyze the type of interrupt.
    uint32_t cause = current_running->user_regs_context.cp0_cause;
    uint32_t ip_mask[8] = {0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};
    uint32_t ip[8];
    int i;
    for(i = 0; i < 8; ++i)
        ip[i] = cause & ip_mask[i];
    if(ip[7]) irq_timer();
    else if(ip[3]) irq_ip3_helper();
    else panic("unknown interrupt.");
}

static void irq_mac()
{
    if(!queue_is_empty(&recv_block_queue))
        do_unblock_one(&recv_block_queue);
    clear_dma_interrupt();
}

static void irq_ip3_helper()
{
    uint32_t int1_sr = *((uint32_t *)INT1_SR);
    if((int1_sr >> 3) & 0x1) irq_mac();
    else panic("unknown ip3.");
}

void set_timer()
{
    asm(
        "mtc0 $0, $9  \n" // $9:CP0_COUNT
        "mtc0 %0, $11 \n" // $11:CP0_COMPARE
        :
        : "r"(timer_increasement)
    );
}

void increase_timer()
{
    asm(
        "mfc0 $t0, $11 \n" // $11:CP0_COMPARE
        "addu $t0, %0  \n"
        "mtc0 $t0, $11 \n"
        :
        : "r"(timer_increasement)
    );
}

static void irq_timer()
{
    ++time_elapsed;
    check_sleeping();
    //vt100_move_cursor(SCREEN_WIDTH - 5 ,SCREEN_HEIGHT - 1);
    //printk("%d", get_timer()/1000);
    set_timer();
    do_scheduler();
}