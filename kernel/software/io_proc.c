#include "sem.h"
#include "syscall.h"
#include "software.h"
#include "sched.h"
#include "common.h"

// todo

semaphore_t io_tasks;
semaphore_t io_done;
semaphore_t free_io_tasks;

io_task_t io_task;

void sw_io_proc()
{
    semaphore_init(&io_tasks, 0);
    semaphore_init(&io_done, 0);
    semaphore_init(&free_io_tasks, 1);

    while(1)
    {
        semaphore_down(&io_tasks);

        if(io_task.type == IO_TASK_WRITE)
        {
            sdwrite(io_task.buffer, io_task.base, io_task.n);
        }
        else
        {
            sdread(io_task.buffer, io_task.base, io_task.n);
        }

        semaphore_up(&io_done);
    }
}

task_info_t io_proc = {(uint32_t)&sw_io_proc, USER_PROCESS, 1, "io_proc"};
task_info_t *info_io_proc = &io_proc;