#include "asm.h"

.equ    KERNEL, 0
.equ    USER,   152

.equ    STACK_TOP_OF_MAIN, 0xa0fe0000

# disable interrupt
# disable interrupt mask
# enable CP0
.equ    INITIAL_CP0_STATUS, 0x1000ff00

.extern current_running

.extern TLB_exception_helper
.extern interrupt_helper
.extern system_call_helper
.extern panic_asm
.extern screen_reflush

LEAF(asm_start)
    li   sp, STACK_TOP_OF_MAIN

    li   k0, INITIAL_CP0_STATUS
    mtc0 k0, CP0_STATUS

    mtc0 zero, CP0_WATCHHI
    mtc0 zero, CP0_WATCHLO

    # only [2:0] can be written
    mfc0 k0, CP0_CONFIG
    li   k1, ~CONFIG_K0
    and  k0, k0, k1
    ori  k0, CONFIG_K0_UNCACHED
    mtc0 k0, CP0_CONFIG

    jr  ra
END(asm_start)

.macro SAVE_REGS_CONTEXT offset
    lw    k0, current_running
    addiu k0, \offset

    # don't need to save $0
    sw $1,  OFFSET_REG1  (k0)
    sw $2,  OFFSET_REG2  (k0)
    sw $3,  OFFSET_REG3  (k0)
    sw $4,  OFFSET_REG4  (k0)
    sw $5,  OFFSET_REG5  (k0)
    sw $6,  OFFSET_REG6  (k0)
    sw $7,  OFFSET_REG7  (k0)
    sw $8,  OFFSET_REG8  (k0)
    sw $9,  OFFSET_REG9  (k0)
    sw $10, OFFSET_REG10 (k0)
    sw $11, OFFSET_REG11 (k0)
    sw $12, OFFSET_REG12 (k0)
    sw $13, OFFSET_REG13 (k0)
    sw $14, OFFSET_REG14 (k0)
    sw $15, OFFSET_REG15 (k0)
    sw $16, OFFSET_REG16 (k0)
    sw $17, OFFSET_REG17 (k0)
    sw $18, OFFSET_REG18 (k0)
    sw $19, OFFSET_REG19 (k0)
    sw $20, OFFSET_REG20 (k0)
    sw $21, OFFSET_REG21 (k0)
    sw $22, OFFSET_REG22 (k0)
    sw $23, OFFSET_REG23 (k0)
    sw $24, OFFSET_REG24 (k0)
    sw $25, OFFSET_REG25 (k0)
    # don't need to save k0, k1
    sw $28, OFFSET_REG28 (k0)
    sw $29, OFFSET_REG29 (k0)
    sw $30, OFFSET_REG30 (k0)
    sw $31, OFFSET_REG31 (k0)

    mfc0 k1, CP0_STATUS
    sw   k1, OFFSET_STATUS   (k0)
    mfc0 k1, CP0_CAUSE
    sw   k1, OFFSET_CAUSE    (k0)
    mfc0 k1, CP0_EPC
    sw   k1, OFFSET_EPC      (k0)
    mfc0 k1, CP0_BADVADDR
    sw   k1, OFFSET_BADVADDR (k0)
    mfhi k1
    sw   k1, OFFSET_HI       (k0)
    mflo k1
    sw   k1, OFFSET_LO       (k0)
.endm

.macro RESTORE_REGS_CONTEXT offset
    lw    k0, current_running
    addiu k0, \offset

    # don't need to restore $0
    lw $1,  OFFSET_REG1  (k0)
    lw $2,  OFFSET_REG2  (k0)
    lw $3,  OFFSET_REG3  (k0)
    lw $4,  OFFSET_REG4  (k0)
    lw $5,  OFFSET_REG5  (k0)
    lw $6,  OFFSET_REG6  (k0)
    lw $7,  OFFSET_REG7  (k0)
    lw $8,  OFFSET_REG8  (k0)
    lw $9,  OFFSET_REG9  (k0)
    lw $10, OFFSET_REG10 (k0)
    lw $11, OFFSET_REG11 (k0)
    lw $12, OFFSET_REG12 (k0)
    lw $13, OFFSET_REG13 (k0)
    lw $14, OFFSET_REG14 (k0)
    lw $15, OFFSET_REG15 (k0)
    lw $16, OFFSET_REG16 (k0)
    lw $17, OFFSET_REG17 (k0)
    lw $18, OFFSET_REG18 (k0)
    lw $19, OFFSET_REG19 (k0)
    lw $20, OFFSET_REG20 (k0)
    lw $21, OFFSET_REG21 (k0)
    lw $22, OFFSET_REG22 (k0)
    lw $23, OFFSET_REG23 (k0)
    lw $24, OFFSET_REG24 (k0)
    lw $25, OFFSET_REG25 (k0)
    # don't need to restore k0, k1
    lw $28, OFFSET_REG28 (k0)
    lw $29, OFFSET_REG29 (k0)
    lw $30, OFFSET_REG30 (k0)
    lw $31, OFFSET_REG31 (k0)

    lw   k1, OFFSET_EPC (k0)
    mtc0 k1, CP0_EPC
    lw   k1, OFFSET_HI  (k0)
    mthi k1
    lw   k1, OFFSET_LO  (k0)
    mtlo k1
.endm

NESTED(do_scheduler, 0, ra)
    .set noat
    SAVE_REGS_CONTEXT(KERNEL)
    .set at

    jal scheduler

    .set noat
    RESTORE_REGS_CONTEXT(KERNEL)
    .set at

    jr ra
END(do_scheduler)

.macro DISABLE_INTERRUPT
    mfc0 k0, CP0_STATUS
    li   k1, ~STATUS_IE
    and  k0, k0, k1
    mtc0 k0, CP0_STATUS
.endm

LEAF(disable_interrupt_public)
    DISABLE_INTERRUPT
    jr ra
END(disable_interrupt_public)

.macro ENABLE_INTERRUPT
    mfc0 k0, CP0_STATUS
    ori  k0, STATUS_IE
    mtc0 k0, CP0_STATUS
.endm

LEAF(enable_interrupt_public)
    ENABLE_INTERRUPT
    jr ra
END(enable_interrupt_public)

.global TLB_refill_handler_begin
.global TLB_refill_handler_end

NESTED(TLB_refill_handler_entry, 0, sp)
TLB_refill_handler_begin:
    # should not exceed 384 Bytes
    .set noat
    SAVE_REGS_CONTEXT(USER)
    .set at

    # load kernel stack pointer
    lw sp, current_running
    addiu sp, KERNEL
    addiu sp, OFFSET_REG29
    lw sp, (sp)

    addiu sp, -4
    li    a0, 1
    la    k0, TLB_exception_helper
    jalr  k0
    addiu sp, 4

    j exception_handler_exit
TLB_refill_handler_end:
END(TLB_refill_handler_entry)

.global exception_handler_begin
.global exception_handler_end

NESTED(exception_handler_entry, 0, sp)
exception_handler_begin:
    # as few as possible instructions here will speed up kernel booting

    # this part will be move to 0x8000_0180
    # different from kernel(0xa080_0000)
    # j will lead to error

    la k0, exception_handler
    jr k0
exception_handler_end:
END(exception_handler_entry)

NESTED(exception_handler, 0, sp)
    .set noat
    SAVE_REGS_CONTEXT(USER)
    .set at

    # load kernel stack pointer
    lw sp, current_running
    addiu sp, KERNEL
    addiu sp, OFFSET_REG29
    lw sp, (sp)

    jal screen_reflush

    mfc0 k0, CP0_CAUSE
    andi k0, CAUSE_EXCCODE
    srl  k0, 2

    li   k1, CAUSE_EXCCODE_INT
    beq  k0, k1, handle_int

    li   k1, CAUSE_EXCCODE_SYSCALL
    beq  k0, k1, handle_syscall

    li   k1, CAUSE_EXCCODE_TLBL
    beq  k0, k1, handle_TLB_exception
    li   k1, CAUSE_EXCCODE_TLBS
    beq  k0, k1, handle_TLB_exception

handle_others:
    addiu sp, -4
    li a0, 0
    jal panic_asm
    addiu sp, 4
    j exception_handler_exit

handle_int:
    jal interrupt_helper
    j exception_handler_exit

handle_syscall:
    jal system_call_helper
    j exception_handler_exit

handle_TLB_exception:
    addiu sp, -4
    li    a0, 0
    jal   TLB_exception_helper
    addiu sp, 4
    j     exception_handler_exit
END(exception_handler)

LEAF(exception_handler_exit)
    lw k0, current_running
    sw sp, OFFSET_REG29 (k0)

    .set noat
    RESTORE_REGS_CONTEXT(USER)
    .set at

    eret
END(exception_handler_exit)

LEAF(invoke_syscall)
    syscall

    jr ra
END(invoke_syscall)

LEAF(init_TLB)
    li t0, 32 # TLB entry number
    li t1, 0
    mtc0 zero, CP0_PAGEMASK
    mtc0 zero, CP0_ENTRYLO0
    mtc0 zero, CP0_ENTRYLO1
    li   t2, 0x80000000 # to assure that unset TLB entry won't hit
                        # necessary if no TLB invalid exception handler
    mtc0 t2, CP0_ENTRYHI
1:
    mtc0  t1, CP0_INDEX
    addiu t1, 1
    tlbwi
    bne t0, t1, 1b

    jr  ra
END(init_TLB)

LEAF(set_TLB_invalid)
    mtc0 a0, CP0_ENTRYHI
    tlbp
    mfc0 t0, CP0_INDEX
    srl  t0, 31
    li   t1, 0x1
    beq  t0, t1, 1f # not in TLB
    mtc0 zero, CP0_ENTRYLO0
    mtc0 zero, CP0_ENTRYLO1
    tlbwi
1:
    jr ra
END(set_TLB_invalid)