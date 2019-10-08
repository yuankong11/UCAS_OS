#include "lock.h"
#include "sched.h"
#include "syscall.h"
#include "screen.h"
#include "debug.h"

lock_t locks[MAX_LOCKS_NUM];

void init_mutex_locks()
{
    int i;
    for(i = 0; i < MAX_LOCKS_NUM; ++i)
        do_mutex_lock_init((mutex_lock_t *)&locks[i]);
}

void init_spin_locks()
{
    int i;
    for(i = 0; i < MAX_LOCKS_NUM; ++i)
        spin_lock_init((spin_lock_t *)&locks[i]);
}

void spin_lock_init(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
    lock->owner = 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    /*
    lock_status_t status;
    status = fetch_and_set(&lock->status);
    while(status == LOCKED)
        status = fetch_and_set(&lock->status);
    */
    if(lock->owner == current_running->pid)
        return;
    while(lock->status == LOCKED);
    ++current_running->lock_num;
    lock->owner  = current_running->pid;
    lock->status = LOCKED;
}

void spin_lock_release(spin_lock_t *lock)
{
    if(lock->owner != current_running->pid)
        return;
    --current_running->lock_num;
    lock->owner  = 0;
    lock->status = UNLOCKED;
}

void do_mutex_lock_init(mutex_lock_t *lock)
{
    lock->status = UNLOCKED;
    lock->owner  = 0;
    queue_init(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    // unnecessary
    /*
    lock_status_t status;
    status = fetch_and_set(&lock->status);
    while(status == LOCKED)
    {
        do_wait(current_running, &lock->block_queue, TASK_BLOCKED);
        status = fetch_and_set(&lock->status);
    }
    */

    if(lock->owner == current_running->pid)
        return;
    while(lock->status == LOCKED)
        do_wait(current_running, &lock->block_queue, TASK_BLOCKED);
    ++current_running->lock_num;
    lock->owner  = current_running->pid;
    lock->status = LOCKED;
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    if(lock->owner != current_running->pid)
        return;
    do_unblock_one(&lock->block_queue);
    --current_running->lock_num;
    lock->owner  = 0;
    lock->status = UNLOCKED;
}
