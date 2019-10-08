#ifndef INCLUDE_SOFTWARE_H_
#define INCLUDE_SOFTWARE_H_

#include "sched.h"
#include "sem.h"
#include "screen.h"

extern task_info_t *info_shell;
extern task_info_t *info_io_proc;
extern task_info_t *info_run;

typedef enum
{
    IO_TASK_READ,
    IO_TASK_WRITE,
} io_task_type_t;

typedef struct io_task
{
    io_task_type_t type;
    unsigned char *buffer;
    uint32_t base;
    int n;
} io_task_t;

extern io_task_t io_task;

extern semaphore_t io_tasks;
extern semaphore_t io_done;
extern semaphore_t free_io_tasks;

extern uint8_t run_buf[2047];

#endif