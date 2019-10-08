#include "barrier.h"
#include "screen.h"

void do_barrier_init(barrier_t *barrier, int goal)
{
    if(goal < 1)
        goal = 1;
    barrier->goal = goal;
    barrier->arrived = 0;
    queue_init(&barrier->wait_queue);
}

void do_barrier_wait(barrier_t *barrier)
{
    static int i = 10;
    ++barrier->arrived;
    if(barrier->arrived == barrier->goal)
    {
        barrier->arrived = 0;
        do_unblock_all(&barrier->wait_queue);
    }
    else
    {
        do_wait(current_running, &barrier->wait_queue, TASK_WAITING);
    }
}