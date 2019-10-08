#include "slist.h"

void slist_init(slist_t l, int length)
{
    int i;
    for(i = 0; i < length; ++i)
    {
        l[i].cursor = i+1;
        l[i].valid = 0;
    }
    l[0].valid = 1;
    l[i-1].cursor = 0; // make a circle
}

int slist_is_insufficient(slist_t l)
{
    return (l[0].cursor == 0);
}

int slist_is_valid(slist_t l, int index, int length)
{
    if(index <= 0 || index >= length)
        return 0;
    return l[index].valid;
}

int slist_alloc(slist_t l)
{
    // alloc a node from slist, return its index
    // return 0 if insufficient
    int t = l[0].cursor;
    if(slist_is_insufficient(l))
        return 0;
    l[0].cursor = l[t].cursor;
    l[t].valid = 1;
    return t;
}

int slist_free(slist_t l, int index, int length)
{
    int t = l[0].cursor;
    if(!slist_is_valid(l, index, length))
        return 0;
    l[0].cursor = index;
    l[index].valid = 0;
    l[index].cursor = t;
    return 1;
}