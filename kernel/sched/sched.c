#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"
#include "debug.h"
#include "slist.h"
#include "syscall.h"

/* allocate memory to pcb */
pcb_t pcb[MAX_TASK_NUM];

/* ready queue to run */
queue_t ready_queue;

/* sleep queue to wait */
queue_t sleep_queue;

/* current running task PCB */
pcb_t *current_running;

/* finite pid, only 64 */
/* related to pcb and memory */
node_t pid_list[MAX_TASK_NUM];

void do_sleep(uint32_t sleep_time)
{
    //ms
    current_running->awakened_time = get_ticks() + sleep_time/TIME_SLICE;
    do_wait(current_running, &sleep_queue, TASK_SLEEPING);
}

void check_sleeping()
{
    pcb_t *p = (pcb_t *)sleep_queue.head;
    while(p)
    {
        if(p->awakened_time <= get_ticks()) // if sleep a very small time, awakened_time will be smaller than get_ticks
        {
            queue_remove(&sleep_queue, p);
            queue_push(&ready_queue, p);
            p->queue = &ready_queue;
            current_running->status = TASK_READY;
        }
        p = p->next;
    }
}

void scheduler(void)
{
    if(!queue_is_empty(&ready_queue))
    {
        pcb_t *p = (pcb_t *)ready_queue.head;
        int max_priority = 1, max_wait_time = 0;
        current_running->cursor_x = screen_cursor_x;
        current_running->cursor_y = screen_cursor_y;
        if(current_running->status == TASK_RUNNING)
            current_running->status = TASK_READY;
        while(p)
        {
            if(p->scheduler_priority > max_priority)
            {
                current_running = p;
                max_priority  = p->scheduler_priority;
                max_wait_time = p->wait_time;
            }
            if(p->scheduler_priority == max_priority && p->wait_time >= max_wait_time)
            {
                current_running = p;
                max_wait_time = p->wait_time;
            }
            ++p->wait_time;
            p = p->next;
        }
        if(current_running->scheduler_priority > 1)
            --current_running->scheduler_priority;
        current_running->wait_time = 0;
        screen_cursor_x = current_running->cursor_x;
        screen_cursor_y = current_running->cursor_y;
        current_running->status = TASK_RUNNING;

        uint32_t EntryHi = current_running->pid;
        asm(
            "mtc0 %0, $10 \n" // EntryHi
            :
            : "r"(EntryHi)
        );
    }
    else
    {
        panic("no task to run.");
    }
}

void do_wait(pcb_t *p, queue_t *queue, task_status_t status)
{
    // delete p from ready_queue, put p into wait_queue q, and modify its status to s
    queue_remove(&ready_queue, p);
    queue_push(queue, p);
    p->queue = queue;
    p->status = status;
    do_scheduler();
}

void do_unblock_one(queue_t *block_queue)
{
    // unblock one from queue, select the one possesses max priority and max wait_time
    if(!queue_is_empty(block_queue))
    {
        pcb_t *p = (pcb_t *)block_queue->head;
        pcb_t *selected;
        int max_priority = 1, max_wait_time = 0;
        while(p)
        {
            if(p->priority > max_priority)
            {
                selected = p;
                max_priority  = p->priority;
                max_wait_time = p->wait_time;
            }
            if(p->priority == max_priority && p->wait_time >= max_wait_time)
            {
                selected = p;
                max_wait_time = p->wait_time;
            }
            ++p->wait_time;
            p = p->next;
        }
        selected->wait_time = 0;
        ++(selected->scheduler_priority);
        queue_remove(block_queue, selected);
        queue_push(&ready_queue, selected);
        selected->queue = &ready_queue;
        selected->status = TASK_READY;
    }
}

void do_unblock_all(queue_t *block_queue)
{
    pcb_t *p;
    while(!queue_is_empty(block_queue))
    {
        p = queue_dequeue(block_queue);
        queue_push(&ready_queue, p);
        p->wait_time = 0;
        ++(p->scheduler_priority);
        p->queue = &ready_queue;
        p->status = TASK_READY;
    }
}

static inline pid_t pid_alloc()
{
    return slist_alloc(pid_list);
}

static inline int pid_free(pid_t pid)
{
    return slist_free(pid_list, pid, MAX_TASK_NUM);
}

static inline int pid_is_valid(pid_t pid)
{
    return slist_is_valid(pid_list, pid, MAX_TASK_NUM);
}

void do_getpid()
{
    current_running->user_regs_context.regs[2] = (uint32_t)current_running->pid;
}

void init_pcb(task_info_t *task)
{
    pid_t pid = pid_alloc();
	if(pid == 0)
	{
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }

    pcb[pid].pid  = pid;
    pcb[pid].name = task->name;
    pcb[pid].type = task->type;
    pcb[pid].lock_num = 0;
    pcb[pid].priority = task->priority;
    pcb[pid].scheduler_priority = pcb[pid].priority;
    pcb[pid].wait_time = 0;
    // when scheduled firstly, exit directly from exception handler
    pcb[pid].kernel_regs_context.regs[REG_RA] = (uint32_t)exception_handler_exit;
    pcb[pid].user_regs_context.cp0_epc = task->entry_point;
    pcb[pid].user_stack_size   = INITIAL_USER_STACK_SIZE;
    pcb[pid].kernel_stack_size = KERNEL_STACK_SIZE;
    pcb[pid].user_regs_context.regs[REG_SP] = USER_STACK_TOP_ADDRESS;
    pcb[pid].kernel_regs_context.regs[REG_SP] = KERNEL_STACK_TOP_ADDRESS - pid*pcb[pid].kernel_stack_size;
    queue_push(&ready_queue, &pcb[pid]);
    pcb[pid].queue = &ready_queue;
    pcb[pid].status = TASK_READY;
    pcb[pid].cursor_x = 0;
    pcb[pid].cursor_y = 0;
    queue_init(&pcb[pid].wait_queue);
    page_dir_init(pcb[pid].page_dir);
}

void kill(pid_t pid)
{
    int i;
    if(pid == 1 || pid_free(pid) == 0) // when pid == 1, it should not be freed
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }
    queue_remove(pcb[pid].queue, &pcb[pid]);
    for(i = 0; i < MAX_LOCKS_NUM && pcb[pid].lock_num > 0; ++i)
    {
        if(locks[i].owner == pid)
        {
            do_unblock_one(&locks[i].block_queue);
            --pcb[pid].lock_num;
            locks[i].owner  = 0;
            locks[i].status = UNLOCKED;
        }
    }
    do_unblock_all(&pcb[pid].wait_queue);
    if(current_running->pid == pid) // do not return
        do_scheduler();
    current_running->user_regs_context.regs[2] = (uint32_t)1;
}

void exit()
{
    kill(current_running->pid); // kill itself
}

void wait(pid_t pid)
{
    if(!pid_is_valid(pid) || pid == current_running->pid)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }
    current_running->user_regs_context.regs[2] = (uint32_t)1;
    do_wait(current_running, &pcb[pid].wait_queue, TASK_WAITING);
}

void show_process()
{
    int i = 0;
    pid_t pid;
    char *status[6] =
    {
        [TASK_BLOCKED ] "TASK_BLOCKED ",
        [TASK_RUNNING ] "TASK_RUNNING ",
        [TASK_READY   ] "TASK_RUNNING ", // other processes is always ready when shell runs
                                         // but ther're as if running together
        [TASK_EXITED  ] "TASK_EXITED  ",
        [TASK_SLEEPING] "TASK_SLEEPING",
        [TASK_WAITING ] "TASK_WAITING ",
    };

    printks("[Process Table]\n");
    printks("       PID         STATUS                NAME\n");
    for(pid = 1; pid < MAX_TASK_NUM; ++pid)
    {
        if(pid_is_valid(pid))
        {
            printks("[%03d]  %03d         %s         %s\n", i, pid, status[pcb[pid].status], pcb[pid].name);
            ++i;
        }
        if(i > 15)
        {
            printks("...\n");
            break;
        }
    }
}