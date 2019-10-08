#include "irq.h"
#include "test.h"
#include "stdio.h"
#include "sched.h"
#include "screen.h"
#include "common.h"
#include "syscall.h"
#include "debug.h"
#include "type.h"
#include "slist.h"
#include "software.h"
#include "mailbox.h"
#include "tlb.h"
#include "mac.h"
#include "fs.h"

void (*printstr)(char *) = (void *)0x8007b980;

static void init_task()
{
	int load_tasks_num;
	task_info_t **load_tasks;

	/* Initialize queue */
	queue_init(&ready_queue);
	queue_init(&sleep_queue);

	/* Initialize PCB of main, scheduled only when ready queue empty */
	pcb[0].pid  = 0;
	pcb[0].name = "main";
	pcb[0].type = KERNEL_PROCESS;
	pcb[0].lock_num = 0;
	/* half for interrupt stack */
	pcb[0].user_stack_size   = USER_STACK_SIZE_OF_MAIN;
	pcb[0].kernel_stack_size = KERNEL_STACK_SIZE;
    pcb[0].kernel_regs_context.regs[REG_SP] = KERNEL_STACK_TOP_ADDRESS;
	/*
	 * sp is initialized in asm_start
	 * epc don't need to be initialized, only used for save & restore
	 * not in any queue, scheduled only when start
	 */
	current_running = &pcb[0];
	pcb[0].status = TASK_RUNNING;
	pcb[0].cursor_x = 0;
	pcb[0].cursor_y = 0;
	queue_init(&pcb[0].wait_queue);

	/* Initialize PCB manager */
	slist_init(pid_list, MAX_TASK_NUM);

	/* Load software */
	init_pcb(info_shell);
	init_pcb(info_io_proc);
}

static void init_exception()
{
	// 1. Get CP0_STATUS
	// 2. Disable all interrupt
	// 3. Copy the level 2 exception handling code to 0x80000180
	// 4. reset CP0_COMPARE & CP0_COUNT register

	/* 1 & 2 have been done in asm_start */
	// general exception handler
	uint32_t *position = (uint32_t *)exception_handler_begin;
	uint32_t *destination = (uint32_t *)0x80000180;
	while((uint32_t)position < (uint32_t)exception_handler_end)
		*(destination++) = *(position++);

	// TLB refill exception handler
	position = (uint32_t *)TLB_refill_handler_begin;
	destination = (uint32_t *)0x80000000;
	while((uint32_t)position < (uint32_t)TLB_refill_handler_end)
		*(destination++) = *(position++);

	set_timer();
}

static void init_syscall(void)
{
	// init system call table.
}

static void init_memory()
{
	init_TLB();
	slist_init(pm_slist, PM_SLIST_SIZE);
	slist_init(pt_pool_slist, PAGE_TABLE_POOL_SLIST_SIZE);
	slist_init(swap_area_slist, SWAP_AREA_SIZE);
}

static void init_driver()
{
	queue_init(&recv_block_queue);
}

static void init_file_system()
{
	// check size here
	uint32_t size[5] =
	{
		sizeof(super_block_t),
		sizeof(inode_t),
		sizeof(dmap_t),
		sizeof(dir_t),
		sizeof(slink_t),
	};
	uint8_t check[5] =
	{
		size[0] != 512,
		size[1] != 64,
		size[2] != 32,
		size[3] != 512,
		size[4] != 512,
	};
	int i;
	for(i = 0; i < 5; ++i)
		if(check[i])
		{
			printks("Error: wrong fs size: %d, %d.\n", i, size[i]);
			while(1);
		}

	slist_init(fd_array_slist, FD_ARRAY_SLIST_SIZE);

	super_block_t s;
	sdread((void *)&s, FS_BASE, SECTOR_SIZE);
	if(s.reserved[78] != MAGIC_NUMBER)
		mkfs();
	else
		mount();
}

static void init_outside_call()
{
	uint32_t *base = (uint32_t *)OUTSIDE_CALL_BASE;
	// no more than 4KB--1K pointers
	base[0] = (uint32_t)sys_move_cursor;
	base[1] = (uint32_t)printf;
	base[2] = (uint32_t)sys_exit;
}

void __attribute__((section(".entry_function"))) _start(void)
{
	printstr("Hello, OS!\n");

	asm_start();
	printk("> [INIT] Asm_start succeeded.\n");

	init_exception();
	printk("> [INIT] Interrupt processing initialization succeeded.\n");

	init_syscall();
	printk("> [INIT] System call initialization successfully.\n");

	init_memory();
	printk("> [INIT] Virtual memory initialization succeeded.\n");

	init_screen();
	vt100_set_color(FORE_COLOR_CYAN); // just for I like it

	init_task();
	printks("> [INIT] Init task succeeded.\n");

	init_outside_call();
	printks("> [INIT] Init outside call succeeded.\n");

	// todo: should not be done by OS
	init_mutex_locks();

	init_file_system();

	screen_clear(0, SCREEN_HEIGHT - 1);

	enable_interrupt_public();

	while(1);

/*
 * Theory is when you know everything but nothing works
 * Practice is when everything works but no one knows why
 * Theory and practice are perfectly combined here
 * Nothing works and no one knows why
 */

}