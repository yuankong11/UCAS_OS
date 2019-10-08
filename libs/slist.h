#ifndef INCLUDE_SLIST_H_
#define INCLUDE_SLIST_H_

/*
 * Static linked list
 */

typedef struct node
{
    int valid;
    int cursor;
} node_t;

// slist_t is a array of node_t
typedef node_t *slist_t;

void slist_init(slist_t l, int length);
int slist_is_insufficient(slist_t l);
int slist_alloc(slist_t l);
int slist_free(slist_t l, int index, int length);
int slist_is_valid(slist_t l, int index, int length);

#endif