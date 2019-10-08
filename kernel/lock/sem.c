#include "sem.h"
#include "stdio.h"

void do_semaphore_init(semaphore_t *s, int value)
{
    if(value < 0)
        value = 0;
    s->value = value;
    queue_init(&s->wait_queue);
}

void do_semaphore_up(semaphore_t *s)
{
    ++s->value;
    if(s->value <= 0)
        do_unblock_one(&s->wait_queue);
}

void do_semaphore_down(semaphore_t *s)
{
    --s->value;
    if(s->value < 0)
        do_wait(current_running, &s->wait_queue, TASK_BLOCKED);
}