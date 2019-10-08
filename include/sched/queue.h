#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include "type.h"
#include "sched.h"

typedef pcb_t item_t;

/*
todo: fix the framework
typedef struct queue
{
    void *head;
    void *tail;
} queue_t;
*/

void queue_init(queue_t *queue);

int queue_is_empty(queue_t *queue);

void queue_push(queue_t *queue, item_t *item);

void queue_push_increasingly(queue_t *queue, item_t *item);

item_t *queue_dequeue(queue_t *queue);

/* remove this item and return next item */
item_t *queue_remove(queue_t *queue, item_t *item);

#endif