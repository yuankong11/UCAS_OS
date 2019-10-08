#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include "type.h"
#include "slist.h"
#include "memory.h"
//#include "lock.h"
//#include "queue.h"

#define MAX_TASK_NUM 16
// main() takes up a position

#define REG_GP 28
#define REG_SP 29
#define REG_FP 30
#define REG_RA 31

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers */
    /* 32 * 4B = 128B */
    /* regs[0] just for alignment */
    uint32_t regs[32];

    /* Saved special registers */
    /* 6 * 4B = 24B */
    uint32_t cp0_status;
    uint32_t cp0_cause;
    uint32_t cp0_epc;
    uint32_t cp0_badvaddr;
    uint32_t hi;
    uint32_t lo;
} regs_context_t; /* 128 + 24 = 152B */

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
    TASK_SLEEPING,
    TASK_WAITING,
} task_status_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

typedef struct queue
{
    void *head;
    void *tail;
} queue_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    regs_context_t kernel_regs_context;
    regs_context_t user_regs_context;

    uint32_t kernel_stack_size; //kernel stack size
    uint32_t user_stack_size;   //user stack size

    /* previous, next pointer */
    struct pcb *prev;
    struct pcb *next;

    /* process id */
    pid_t pid;

    /* process priority */
    int priority;
    int scheduler_priority;

    /* process wait time */
    int wait_time;

    /* awakened time */
    /* ticks */
    uint32_t awakened_time;

    /* process name(optional) */
    char *name;

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | EXITED*/
    task_status_t status;

    /* lock */
    int lock_num;

    /* queue that the pcb's in */
    queue_t *queue;

    /* wait queue: tasks that's waiting this task */
    queue_t wait_queue;

    /* page directory */
    page_dir_t page_dir;

    /* cursor position */
    int cursor_x;
    int cursor_y;
} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    uint32_t entry_point;
    task_type_t type;
    int priority;
    char *name;
} task_info_t;

/* allocate memory to pcb */
extern pcb_t pcb[MAX_TASK_NUM];

/* ready queue to run */
extern queue_t ready_queue;

/* sleep queue to wait */
extern queue_t sleep_queue;

/* current running task PCB */
extern pcb_t *current_running;

/* finite pid, only 16 */
/* related to pcb and memory */
extern node_t pid_list[MAX_TASK_NUM];

void scheduler(void);
void do_scheduler(void);
void exception_handler_exit();
void do_sleep(uint32_t);

void do_unblock_one(queue_t *);
void do_unblock_all(queue_t *);

void do_sleep(uint32_t);
void check_sleeping();

void do_getpid();
void init_pcb(task_info_t *tasks);
void show_process();
void init_pcb(task_info_t *task);
void kill(pid_t pid);
void exit();
void do_wait(pcb_t *p, queue_t *queue, task_status_t status);
void wait(pid_t pid);

#endif