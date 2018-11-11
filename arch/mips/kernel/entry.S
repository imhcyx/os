#include "asm.h"

.global current_running

.equ    KERNEL, 0
.equ    USER,   156

.macro STI
    .set noat
    mfc0    k0, CP0_STATUS
    ori     k0, STATUS_IE
    mtc0    k0, CP0_STATUS
    .set at
.endm

.macro CLI
    .set noat
    mfc0    k0, CP0_STATUS
    li      k1, ~STATUS_IE
    and     k0, k1
    mtc0    k0, CP0_STATUS
    .set at
.endm

LEAF(asm_start)
    mtc0    zero, CP0_STATUS
    mtc0    zero, CP0_WATCHLO
    mtc0    zero, CP0_WATCHHI

    mfc0    t0, CP0_CONFIG
    and     t0, ~0x7
    ori     t0, 0x2
    mtc0    t0, CP0_CONFIG
    
    jr  ra
END(asm_start)

.macro SAVE_CONTEXT offset
    .set noat
    lw      k0, current_running
    addiu   k0, \offset
    sw      $0, 0(k0)
    sw      $1, 4(k0)
    sw      $2, 8(k0)
    sw      $3, 12(k0)
    sw      $4, 16(k0)
    sw      $5, 20(k0)
    sw      $6, 24(k0)
    sw      $7, 28(k0)
    sw      $8, 32(k0)
    sw      $9, 36(k0)
    sw      $10, 40(k0)
    sw      $11, 44(k0)
    sw      $12, 48(k0)
    sw      $13, 52(k0)
    sw      $14, 56(k0)
    sw      $15, 60(k0)
    sw      $16, 64(k0)
    sw      $17, 68(k0)
    sw      $18, 72(k0)
    sw      $19, 76(k0)
    sw      $20, 80(k0)
    sw      $21, 84(k0)
    sw      $22, 88(k0)
    sw      $23, 92(k0)
    sw      $24, 96(k0)
    sw      $25, 100(k0)
    // k0 k1 ignored
    sw      $28, 112(k0)
    sw      $29, 116(k0)
    sw      $30, 120(k0)
    sw      $31, 124(k0)
    mfc0    k1, CP0_STATUS
    sw      k1, 128(k0)
    mfhi    k1
    sw      k1, 132(k0)
    mflo    k1
    sw      k1, 136(k0)
    mfc0    k1, CP0_BADVADDR
    sw      k1, 140(k0)
    mfc0    k1, CP0_CAUSE
    sw      k1, 144(k0)
    mfc0    k1, CP0_EPC
    sw      k1, 148(k0)
    .set at
.endm

.macro RESTORE_CONTEXT offset
    .set noat
    lw      k0, current_running
    addiu   k0, \offset
    lw      $1, 4(k0)
    lw      $2, 8(k0)
    lw      $3, 12(k0)
    lw      $4, 16(k0)
    lw      $5, 20(k0)
    lw      $6, 24(k0)
    lw      $7, 28(k0)
    lw      $8, 32(k0)
    lw      $9, 36(k0)
    lw      $10, 40(k0)
    lw      $11, 44(k0)
    lw      $12, 48(k0)
    lw      $13, 52(k0)
    lw      $14, 56(k0)
    lw      $15, 60(k0)
    lw      $16, 64(k0)
    lw      $17, 68(k0)
    lw      $18, 72(k0)
    lw      $19, 76(k0)
    lw      $20, 80(k0)
    lw      $21, 84(k0)
    lw      $22, 88(k0)
    lw      $23, 92(k0)
    lw      $24, 96(k0)
    lw      $25, 100(k0)
    // k0 k1 ignored
    lw      $28, 112(k0)
    lw      $29, 116(k0)
    lw      $30, 120(k0)
    lw      $31, 124(k0)
    lw      k1, 132(k0)
    mthi    k1
    lw      k1, 136(k0)
    mtlo    k1
    lw      k1, 148(k0)
    mtc0    k1, CP0_EPC
    .set at
 .endm

// DEPRECATED in preemptive kernel
NESTED(do_scheduler, 0, ra)
    SAVE_CONTEXT(KERNEL)
    jal     scheduler
    RESTORE_CONTEXT(KERNEL)
    jr      ra
END(do_scheduler)

.global exception_handler_begin
.global exception_handler_end

NESTED(exception_handler_entry, 0, sp)   
exception_handler_begin:
    CLI
    la      k0, exception_handler
    jr      k0
exception_handler_end:
END(exception_handler_entry)

NESTED(exception_handler, 0, sp)   

    mfc0    k0, CP0_CAUSE
    andi    k0, CAUSE_EXCCODE
    beqz    k0, exception_handler_is_int
    li      k1, 8<<2
    beq     k0, k1, exception_handler_is_syscall
    SAVE_CONTEXT(KERNEL)
    jal     other_exception_handler
    b       exception_handler_exit

exception_handler_is_int:
    SAVE_CONTEXT(KERNEL)
    jal     interrupt_helper
    b       exception_handler_exit

exception_handler_is_syscall:
    mfc0    k0, CP0_EPC
    addiu   k0, 4
    mtc0    k0, CP0_EPC
    SAVE_CONTEXT(KERNEL)
    jal     system_call_helper
    b       exception_handler_exit

exception_handler_exit:
    RESTORE_CONTEXT(KERNEL)
    STI
    move    k0, zero
    move    k1, zero
    eret

END(exception_handler)
