#include "string.h"
#include "mailbox.h"
#include "cond.h"
#include "syscall.h"

static mailbox_t mboxs[MAX_BOX_NUM];

static mutex_lock_t lock; // used when open and close, send and recv use sem to speed up

void mbox_init()
{
    int i;
    for(i = 0; i < MAX_BOX_NUM; ++i)
        mboxs[i].valid = 0;
    mutex_lock_init(&lock);
}

mailbox_t *mbox_open(char *name)
{
    int i;

    mutex_lock_acquire(&lock);
    for(i = 0; i < MAX_BOX_NUM; ++i)
    {
        if(!mboxs[i].valid)
            continue;
        if(strcmp(mboxs[i].name, name) == 0)
        {
            ++mboxs[i].user_num;
            mutex_lock_release(&lock);
            return &mboxs[i];
        }
    }
    for(i = 0; i < MAX_BOX_NUM && mboxs[i].valid; ++i)
        ;
    if(i == MAX_BOX_NUM)
    {
        mutex_lock_release(&lock);
        return NULL;
    }
    mboxs[i].valid = 1;
    mboxs[i].name = name;
    mboxs[i].user_num = 1;
    condition_init(&mboxs[i].empty);
    condition_init(&mboxs[i].full);
    mboxs[i].head = mboxs[i].tail = 0;
    mutex_lock_release(&lock);
    return &mboxs[i];
}

void mbox_close(mailbox_t *mailbox)
{
    mutex_lock_acquire(&lock);
    --mailbox->user_num;
    if(mailbox->user_num == 0)
        mailbox->valid = 0;
    mutex_lock_release(&lock);
}

void mbox_send(mailbox_t *mailbox, void *msg, int msg_length)
{
    int i;
    mutex_lock_acquire(&lock);
    for(i = 0; i < msg_length; ++i)
    {
        while(mailbox->head == mailbox->tail + 1) // full
            condition_wait(&mailbox->full, &lock);
        mailbox->data_buffer[mailbox->tail] = *((uint8_t *)msg);
        mailbox->tail = (mailbox->tail + 1) % MAX_BUFFER_SIZE;
        ++msg;
        condition_signal(&mailbox->empty);
    }
    mutex_lock_release(&lock);
}

void mbox_recv(mailbox_t *mailbox, void *msg, int msg_length)
{
    int i;
    mutex_lock_acquire(&lock);
    for(i = 0; i < msg_length; ++i)
    {
        while(mailbox->head == mailbox->tail) // empty
            condition_wait(&mailbox->empty, &lock);
        *((uint8_t *)msg) = mailbox->data_buffer[mailbox->head];
        mailbox->head = (mailbox->head + 1) % MAX_BUFFER_SIZE;
        ++msg;
        condition_signal(&mailbox->full);
    }
    mutex_lock_release(&lock);
}