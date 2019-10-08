#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include "type.h"
#include "sync.h"
#include "queue.h"

#define IGNORE 0

#define SYSCALL_SHOW_PROCESS 0
#define SYSCALL_SPAWN 1
#define SYSCALL_KILL 2
#define SYSCALL_EXIT 3
#define SYSCALL_WAIT 4
#define SYSCALL_SLEEP 5
#define SYSCALL_GETPID 6

#define SYSCALL_BLOCK 10
#define SYSCALL_UNBLOCK_ONE 11
#define SYSCALL_UNBLOCK_ALL 12

#define SYSCALL_WRITE 20
#define SYSCALL_READ 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_CLEAR 24

#define SYSCALL_MUTEX_LOCK_INIT 30
#define SYSCALL_MUTEX_LOCK_ACQUIRE 31
#define SYSCALL_MUTEX_LOCK_RELEASE 32

#define SYSCALL_SEMAPHORE_INIT 33
#define SYSCALL_SEMAPHORE_UP 34
#define SYSCALL_SEMAPHORE_DOWN 35

#define SYSCALL_COND_INIT 36
#define SYSCALL_COND_WAIT 37
#define SYSCALL_COND_SIGNAL 38
#define SYSCALL_COND_BROADCAST 39

#define SYSCALL_BARRIER_INIT 40
#define SYSCALL_BARRIER_WAIT 41

#define SYSCALL_INIT_MAC 50
#define SYSCALL_NET_SEND 51
#define SYSCALL_NET_RECV 52
#define SYSCALL_WAIT_RECV 53

#define SYSCALL_MKFS 60
#define SYSCALL_MOUNT 61
#define SYSCALL_STATFS 62
#define SYSCALL_GET_DIR_NAME 63
#define SYSCALL_MKDIR 64
#define SYSCALL_RMDIR 65
#define SYSCALL_LS 66
#define SYSCALL_CD 67
#define SYSCALL_TOUCH 68

#define SYSCALL_FOPEN 70
#define SYSCALL_FCLOSE 71
#define SYSCALL_FREAD 72
#define SYSCALL_FWRITE 73
#define SYSCALL_FSEEK 74

void system_call_helper();
extern int invoke_syscall(uint32_t, uint32_t, uint32_t, uint32_t);

void sys_show_process();
int sys_spawn(task_info_t *);
int sys_kill(pid_t);
void sys_exit();
int sys_wait(pid_t);
void sys_sleep(uint32_t);
pid_t sys_getpid();

void sys_block(queue_t *);
void sys_unblock_one(queue_t *);
void sys_unblock_all(queue_t *);

void sys_write(char *);
char sys_read();
void sys_move_cursor(int, int);
void sys_reflush();
void sys_clear(int line1, int line2);

void mutex_lock_init(mutex_lock_t *);
void mutex_lock_acquire(mutex_lock_t *);
void mutex_lock_release(mutex_lock_t *);

void semaphore_init(semaphore_t *, int );
void semaphore_up(semaphore_t *);
void semaphore_down(semaphore_t *);

void condition_init(condition_t *);
void condition_wait(condition_t *, mutex_lock_t *);
void condition_signal(condition_t *);
void condition_broadcast(condition_t *);

void barrier_init(barrier_t *, int);
void barrier_wait(barrier_t *);

void sys_init_mac();
void sys_net_send();
void sys_net_recv();
void sys_net_wait_recv();

void sys_mkfs();
void sys_mount();
void sys_statfs();
char *sys_get_curr_dir_name();
void sys_mkdir(char *s);
void sys_rmdir(char *s);
void sys_ls();
void sys_cd(char *s);
void sys_touch(char *s);

int sys_fopen(char *s, int access);
int sys_fclose(int fd);
int sys_fread(int fd, uint8_t *buf, uint32_t size);
int sys_fwrite(int fd, uint8_t *buf, uint32_t size);
void sys_fseek(int fd, uint32_t offset);

#endif