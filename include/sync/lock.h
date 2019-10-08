#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_

#include "queue.h"

#define MAX_LOCKS_NUM 16

typedef enum {
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef struct spin_lock
{
    lock_status_t status;
    pid_t owner;
} spin_lock_t;

typedef struct mutex_lock
{
    lock_status_t status;
    pid_t owner;
    queue_t block_queue;
} mutex_lock_t;

typedef mutex_lock_t lock_t;

extern lock_t locks[MAX_LOCKS_NUM];

/* init lock */
void spin_lock_init(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

void do_mutex_lock_init(mutex_lock_t *lock);
void do_mutex_lock_acquire(mutex_lock_t *lock);
void do_mutex_lock_release(mutex_lock_t *lock);

void init_mutex_locks();
void init_spin_locks();

#endif
