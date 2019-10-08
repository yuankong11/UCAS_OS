#include "queue.h"

void queue_init(queue_t *queue)
{
    queue->head = queue->tail = NULL;
}

int queue_is_empty(queue_t *queue)
{
    return (queue->head == NULL);
}

void queue_push(queue_t *queue, item_t *item)
{
    /* queue is empty */
    if (queue->head == NULL)
    {
        queue->head = (void *)item;
        queue->tail = (void *)item;
        item->next = NULL;
        item->prev = NULL;
    }
    else
    {
        ((item_t *)(queue->tail))->next = item;
        item->next = NULL;
        item->prev = (item_t *)queue->tail;
        queue->tail = (void *)item;
    }
}

void queue_push_increasingly(queue_t *queue, item_t *item)
{
    /*
     * push item in increasing order of priority
     * the queue should be in increasing order from tail to head
     */
    item_t *t;
    if(queue->head == NULL)
    {
        queue->head = (void *)item;
        queue->tail = (void *)item;
        item->next = NULL;
        item->prev = NULL;
    }
    else
    {
        for(t = (item_t *)queue->head; t && t->priority > item->priority; t = t->next)
            ;
        if((void *)t == queue->head)
        {
            t->prev = item;
            item->next = t;
            item->prev = NULL;
            queue->head = (void *)item;
        }
        else if(t == NULL)
        {
            t = (item_t *)queue->tail;
            t->next = item;
            item->prev = t;
            item->next = NULL;
            queue->tail = (void *)item;
        }
        else
        {
            t->prev->next = item;
            item->prev = t->prev;
            t->prev = item;
            item->next = t;
        }
    }
}

item_t *queue_dequeue(queue_t *queue)
{
    item_t *head = (item_t *)queue->head;

    /* this queue only has one item */
    if (head->next == NULL)
        queue->head = queue->tail = NULL;
    else
    {
        queue->head = (void *)( ((item_t *)(queue->head))->next );
        ((item_t *)(queue->head))->prev = NULL;
        head->prev = NULL;
        head->next = NULL;
    }

    return head;
}

/* remove this item and return next item */
item_t *queue_remove(queue_t *queue, item_t *item)
{
    item_t *next = item->next;

    if ((void *)item == queue->head && (void *)item == queue->tail)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else
    {
        if ((void *)item == queue->head)
        {
            queue->head = (void *)item->next;
            ((item_t *)(queue->head))->prev = NULL;
        }
        else if ((void *)item == queue->tail)
        {
            queue->tail = (void *)item->prev;
            ((item_t *)(queue->tail))->next = NULL;
        }
        else
        {
            ((item_t *)(item->prev))->next = (void *)item->next;
            ((item_t *)(item->next))->prev = (void *)item->prev;
        }
        item->prev = NULL;
        item->next = NULL;
    }

    return (void *)next;
}