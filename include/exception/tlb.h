#ifndef INCLUDE_TLB_H_
#define INCLUDE_TLB_H_

#include "exception.h"

void init_TLB();
void TLB_exception_helper(int is_TLB_refill);

#endif