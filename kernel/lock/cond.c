#include "cond.h"
#include "lock.h"

void do_condition_init(condition_t *condition)
{
    condition->waiting_num = 0;
    queue_init(&condition->wait_queue);
}

void do_condition_wait(condition_t *condition, mutex_lock_t *lock)
{
    ++condition->waiting_num;
    do_mutex_lock_release(lock);
    do_wait(current_running, &condition->wait_queue, TASK_BLOCKED);
    do_mutex_lock_acquire(lock);
}

void do_condition_signal(condition_t *condition)
{
    if(condition->waiting_num > 0)
    {
        --condition->waiting_num;
        do_unblock_one(&condition->wait_queue);
    }
}

void do_condition_broadcast(condition_t *condition)
{
    if(condition->waiting_num > 0)
    {
        condition->waiting_num = 0;
        do_unblock_all(&condition->wait_queue);
    }
}