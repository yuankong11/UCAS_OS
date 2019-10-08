#include "lock.h"
#include "sched.h"
#include "common.h"
#include "screen.h"
#include "debug.h"
#include "sem.h"
#include "cond.h"
#include "barrier.h"
#include "common.h"
#include "syscall.h"
#include "mac.h"
#include "fs.h"

void system_call_helper()
{
    int fn   = current_running->user_regs_context.regs[4];
    int arg1 = current_running->user_regs_context.regs[5];
    int arg2 = current_running->user_regs_context.regs[6];
    int arg3 = current_running->user_regs_context.regs[7];
    current_running->user_regs_context.cp0_epc += 4;
    switch (fn)
    {
        case SYSCALL_SHOW_PROCESS: show_process(); break;
        case SYSCALL_SPAWN:        init_pcb((task_info_t *)arg1); break;
        case SYSCALL_KILL:         kill((pid_t)arg1); break;
        case SYSCALL_EXIT:         exit(); break;
        case SYSCALL_WAIT:         wait((pid_t)arg1); break;
        case SYSCALL_SLEEP:        do_sleep((int)arg1); break;
        case SYSCALL_GETPID:       do_getpid(); break;

        case SYSCALL_WRITE:   screen_write((char *)arg1); break;
        case SYSCALL_READ:    read_uart_ch(); break;
        case SYSCALL_CURSOR:  screen_move_cursor((int)arg1, (int)arg2); break;
        case SYSCALL_REFLUSH: screen_reflush(); break;
        case SYSCALL_CLEAR:   screen_clear((int)arg1, (int)arg2); break;

        case SYSCALL_MUTEX_LOCK_INIT:    do_mutex_lock_init((mutex_lock_t *)arg1); break;
        case SYSCALL_MUTEX_LOCK_ACQUIRE: do_mutex_lock_acquire((mutex_lock_t *)arg1); break;
        case SYSCALL_MUTEX_LOCK_RELEASE: do_mutex_lock_release((mutex_lock_t *)arg1); break;

        case SYSCALL_SEMAPHORE_INIT: do_semaphore_init((semaphore_t *)arg1, (int)arg2); break;
        case SYSCALL_SEMAPHORE_UP:   do_semaphore_up((semaphore_t *)arg1); break;
        case SYSCALL_SEMAPHORE_DOWN: do_semaphore_down((semaphore_t *)arg1); break;

        case SYSCALL_COND_INIT:      do_condition_init((condition_t *)arg1); break;
        case SYSCALL_COND_WAIT:      do_condition_wait((condition_t *)arg1, (mutex_lock_t *)arg2); break;
        case SYSCALL_COND_SIGNAL:    do_condition_signal((condition_t *)arg1); break;
        case SYSCALL_COND_BROADCAST: do_condition_broadcast((condition_t *)arg1); break;

        case SYSCALL_BARRIER_INIT: do_barrier_init((barrier_t *)arg1, (int)arg2); break;
        case SYSCALL_BARRIER_WAIT: do_barrier_wait((barrier_t *)arg1); break;

        case SYSCALL_INIT_MAC:  do_init_mac(); break;
        case SYSCALL_NET_SEND:  do_net_send(); break;
        case SYSCALL_NET_RECV:  do_net_recv(); break;
        case SYSCALL_WAIT_RECV: do_wait_recv_package(); break;

        case SYSCALL_MKFS:         mkfs(); break;
        case SYSCALL_MOUNT:        mount(); break;
        case SYSCALL_STATFS:       statfs(); break;
        case SYSCALL_GET_DIR_NAME: get_curr_dir_name(); break;
        case SYSCALL_MKDIR:        mkdir((char *)arg1); break;
        case SYSCALL_RMDIR:        rmdir((char *)arg1); break;
        case SYSCALL_LS:           ls(); break;
        case SYSCALL_CD:           cd((char *)arg1); break;
        case SYSCALL_TOUCH:        touch((char *)arg1); break;

        case SYSCALL_FOPEN:  fopen((char *)arg1, (int)arg2); break;
        case SYSCALL_FCLOSE: fclose((int)arg1); break;
        case SYSCALL_FREAD:  fread((int)arg1, (uint8_t *)arg2, (uint32_t)arg3); break;
        case SYSCALL_FWRITE: fwrite((int)arg1, (uint8_t *)arg2, (uint32_t)arg3); break;
        case SYSCALL_FSEEK:  fseek((int)arg1, (uint32_t)arg2); break;

        default: panic("unknown syscall.");
    }
}

pid_t sys_getpid()
{
    invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE);
}

void barrier_init(barrier_t *barrier, int goal)
{
    invoke_syscall(SYSCALL_BARRIER_INIT, (uint32_t)barrier, (uint32_t)goal, IGNORE);
}

void barrier_wait(barrier_t *barrier)
{
    invoke_syscall(SYSCALL_BARRIER_WAIT, (uint32_t)barrier, IGNORE, IGNORE);
}

void condition_init(condition_t *condition)
{
    invoke_syscall(SYSCALL_COND_INIT, (uint32_t)condition, IGNORE, IGNORE);
}

void condition_wait(condition_t *condition, mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_COND_WAIT, (uint32_t)condition, (uint32_t)lock, IGNORE);
}

void condition_signal(condition_t *condition)
{
    invoke_syscall(SYSCALL_COND_SIGNAL, (uint32_t)condition, IGNORE, IGNORE);
}

void condition_broadcast(condition_t *condition)
{
    invoke_syscall(SYSCALL_COND_BROADCAST, (uint32_t)condition, IGNORE, IGNORE);
}

void semaphore_init(semaphore_t *s, int value)
{
    invoke_syscall(SYSCALL_SEMAPHORE_INIT, (uint32_t)s, (uint32_t)value, IGNORE);
}

void semaphore_up(semaphore_t *s)
{
    invoke_syscall(SYSCALL_SEMAPHORE_UP, (uint32_t)s, IGNORE, IGNORE);
}

void semaphore_down(semaphore_t *s)
{
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN, (uint32_t)s, IGNORE, IGNORE);
}

int sys_spawn(task_info_t *task)
{
    invoke_syscall(SYSCALL_SPAWN, (uint32_t)task, IGNORE, IGNORE);
}

int sys_kill(pid_t pid)
{
    invoke_syscall(SYSCALL_KILL, (uint32_t)pid, IGNORE, IGNORE);
}

void sys_exit()
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

int sys_wait(pid_t pid)
{
    invoke_syscall(SYSCALL_WAIT, (uint32_t)pid, IGNORE, IGNORE);
}

void sys_clear(int line1, int line2)
{
    // clear screen from line1 to line2
    // line start from 0
    invoke_syscall(SYSCALL_CLEAR, (uint32_t)line1, (uint32_t)line2, IGNORE);
}

void sys_show_process()
{
    invoke_syscall(SYSCALL_SHOW_PROCESS, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, (uint32_t)time, IGNORE, IGNORE);
}

void sys_block(queue_t *queue)
{
    invoke_syscall(SYSCALL_BLOCK, (uint32_t)queue, IGNORE, IGNORE);
}

void sys_unblock_one(queue_t *queue)
{
    invoke_syscall(SYSCALL_UNBLOCK_ONE, (uint32_t)queue, IGNORE, IGNORE);
}

void sys_unblock_all(queue_t *queue)
{
    invoke_syscall(SYSCALL_UNBLOCK_ALL, (uint32_t)queue, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uint32_t)buff, IGNORE, IGNORE);
}

char sys_read()
{
    invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, (uint32_t)x, (uint32_t)y, IGNORE);
}

void mutex_lock_init(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_INIT, (uint32_t)lock, IGNORE, IGNORE);
}

void mutex_lock_acquire(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_ACQUIRE, (uint32_t)lock, IGNORE, IGNORE);
}

void mutex_lock_release(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_RELEASE, (uint32_t)lock, IGNORE, IGNORE);
}

void sys_init_mac()
{
    invoke_syscall(SYSCALL_INIT_MAC, IGNORE, IGNORE, IGNORE);
}

void sys_net_send()
{
    invoke_syscall(SYSCALL_NET_SEND, IGNORE, IGNORE, IGNORE);
}

void sys_net_recv(uint32_t *buffer)
{
    invoke_syscall(SYSCALL_NET_RECV, IGNORE, IGNORE, IGNORE);
}

void sys_net_wait_recv()
{
    invoke_syscall(SYSCALL_WAIT_RECV, IGNORE, IGNORE ,IGNORE);
}

void sys_mkfs()
{
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE);
}

void sys_mount()
{
    invoke_syscall(SYSCALL_MOUNT, IGNORE, IGNORE, IGNORE);
}

void sys_statfs()
{
    invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE);
}

char *sys_get_curr_dir_name()
{
    invoke_syscall(SYSCALL_GET_DIR_NAME, IGNORE, IGNORE, IGNORE);
}

void sys_mkdir(char *s)
{
    invoke_syscall(SYSCALL_MKDIR, (uint32_t)s, IGNORE, IGNORE);
}

void sys_rmdir(char *s)
{
    invoke_syscall(SYSCALL_RMDIR, (uint32_t)s, IGNORE, IGNORE);
}

void sys_ls()
{
    invoke_syscall(SYSCALL_LS, IGNORE, IGNORE, IGNORE);
}

void sys_cd(char *s)
{
    invoke_syscall(SYSCALL_CD, (uint32_t)s, IGNORE, IGNORE);
}

void sys_touch(char *s)
{
    invoke_syscall(SYSCALL_TOUCH, (uint32_t)s, IGNORE, IGNORE);
}

int sys_fopen(char *s, int access)
{
    invoke_syscall(SYSCALL_FOPEN, (uint32_t)s, (uint32_t)access, IGNORE);
}

int sys_fclose(int fd)
{
    invoke_syscall(SYSCALL_FCLOSE, (uint32_t)fd, IGNORE, IGNORE);
}

int sys_fread(int fd, uint8_t *buf, uint32_t size)
{
    invoke_syscall(SYSCALL_FREAD, (uint32_t)fd, (uint32_t)buf, (uint32_t)size);
}

int sys_fwrite(int fd, uint8_t *buf, uint32_t size)
{
    invoke_syscall(SYSCALL_FWRITE, (uint32_t)fd, (uint32_t)buf, (uint32_t)size);
}

void sys_fseek(int fd, uint32_t offset)
{
    invoke_syscall(SYSCALL_FSEEK, (uint32_t)fd, (uint32_t)offset, IGNORE);
}