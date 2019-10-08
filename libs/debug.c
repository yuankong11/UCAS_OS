#include "stdio.h"
#include "screen.h"
#include "type.h"
#include "sched.h"

void break_point(void){}

// kernel panic with context saved & interrupt closed
void panic(char *s)
{
    int i;
    screen_clear(0, SCREEN_HEIGHT-1);
    vt100_move_cursor(1, 1);
    printk("* * * * * * * * PANIC * * * * * * * *\n");

    printk("Error: %s\n", s);
    printk("Task: %s, Pid: %d\n", current_running->name, current_running->pid);
    printk("STATUS:0x%08x, CAUSE:0x%08x\n",
            current_running->user_regs_context.cp0_status, current_running->user_regs_context.cp0_cause
    );
    printk("EPC:0x%08x, BADVADDR:0x%08x\n",
            current_running->user_regs_context.cp0_epc, current_running->user_regs_context.cp0_badvaddr
    );
    for(i = 0; i < 32; i+=2)
    {
        printk("Reg%02d: 0x%08x, Reg%02d: 0x%08x\n",
                i,   current_running->user_regs_context.regs[i],
                i+1, current_running->user_regs_context.regs[i+1]
        );
    }

    while(1);
}

void panic_asm(int number)
{
    if(number == 0) panic("Unknown exception.");
    else panic("Error in asm.");
}

void debug_cursor()
{
    static i = SCREEN_HEIGHT + 1;
    vt100_move_cursor(1, i++);
}

void debug_wait(int wait_time)
{
    int i;
    for(i = 0; i < wait_time; ++i);
}