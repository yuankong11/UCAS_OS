#include "asm.h"

.extern print_error

# uint32_t fetch_and_set(uint32_t *address)
LEAF(fetch_and_set)
fetch:
    .set noreorder
    ll v0, (a0)
    li t0, 1
    sc t0, (a0)
    beqz t0, fetch
    nop
    jr ra
    nop
    .set reorder
END(fetch_and_set)