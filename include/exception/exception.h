#ifndef INCLUDE_EXCEPTION_H_
#define INCLUDE_EXCEPTION_H_

/* ERROR code */
enum ExcCode
{
    /* 14, 16-22, 24-31 is reserver ExcCode */
    INT,       // 0
    MOD,       // 1
    TLBL,      // 2
    TLBS,      // 3
    ADEL,      // 4
    ADES,      // 5
    IBE,       // 6
    DBE,       // 7
    SYS,       // 8
    BP,        // 9
    RI,        // 10
    CPU,       // 11
    OV,        // 12
    TR,        // 13
    FPE = 15,  // 15
    WATCH = 23 // 23
};

#define ExcCode 0x7c

/* BEV = 0 */
// Exception Enter Vector
#define BEV0_EBASE 0x80000000
#define BEV0_OFFSET 0x180

/* BEV = 1 */
#define BEV1_EBASE 0xbfc00000
#define BEV1_OFFSET 0x380

/* exception handler entery */
void exception_handler_entry();
void exception_handler_begin();
void exception_handler_end();
void TLB_refill_handler_entry();
void TLB_refill_handler_begin();
void TLB_refill_handler_end();

void handle_int(void);
void handle_syscall(void);
void handle_others(void);

void disable_interrupt_public();
void enable_interrupt_public();

#endif