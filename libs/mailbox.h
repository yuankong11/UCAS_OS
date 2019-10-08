#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include "cond.h"

#define MAX_BOX_NUM 8
#define MAX_BUFFER_SIZE 256 // words

typedef struct mailbox
{
    int valid;
    char *name;
    int user_num;
    uint8_t data_buffer[MAX_BUFFER_SIZE];
    condition_t empty, full;
    int head, tail;
} mailbox_t;


void mbox_init();
mailbox_t *mbox_open(char *);
void mbox_close(mailbox_t *);
void mbox_send(mailbox_t *, void *, int);
void mbox_recv(mailbox_t *, void *, int);

#endif