# trap.S
#include "trapframe.h"
.section .text
.global trap_vector
.global trap_return
.align 4

trap_vector:
    # user -> kernel: sp 切到 mscratch 保存的内核栈顶，mscratch 留下原用户 sp
    csrrw sp, mscratch, sp
    addi sp, sp, -TRAPFRAME_SIZE

    sd ra, TF_RA_OFF(sp)
    sd gp, TF_GP_OFF(sp)
    sd tp, TF_TP_OFF(sp)
    sd t0, TF_T0_OFF(sp)
    sd t1, TF_T1_OFF(sp)
    sd t2, TF_T2_OFF(sp)
    sd s0, TF_S0_OFF(sp)
    sd s1, TF_S1_OFF(sp)
    sd a0, TF_A0_OFF(sp)
    sd a1, TF_A1_OFF(sp)
    sd a2, TF_A2_OFF(sp)
    sd a3, TF_A3_OFF(sp)
    sd a4, TF_A4_OFF(sp)
    sd a5, TF_A5_OFF(sp)
    sd a6, TF_A6_OFF(sp)
    sd a7, TF_A7_OFF(sp)
    sd s2, TF_S2_OFF(sp)
    sd s3, TF_S3_OFF(sp)
    sd s4, TF_S4_OFF(sp)
    sd s5, TF_S5_OFF(sp)
    sd s6, TF_S6_OFF(sp)
    sd s7, TF_S7_OFF(sp)
    sd s8, TF_S8_OFF(sp)
    sd s9, TF_S9_OFF(sp)
    sd s10, TF_S10_OFF(sp)
    sd s11, TF_S11_OFF(sp)
    sd t3, TF_T3_OFF(sp)
    sd t4, TF_T4_OFF(sp)
    sd t5, TF_T5_OFF(sp)
    sd t6, TF_T6_OFF(sp)

    csrr t0, mepc
    sd t0, TF_EPC_OFF(sp)
    csrr t0, mscratch
    sd t0, TF_USER_SP_OFF(sp)

    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    ld   a3, TF_A0_OFF(sp)
    ld   a4, TF_A1_OFF(sp)
    ld   a5, TF_A7_OFF(sp)
    mv   a6, sp

    call trap_handler
    sd   a0, TF_EPC_OFF(sp)
    csrw mepc, a0

trap_return:
    ld t2, TF_EPC_OFF(sp)
    csrw mepc, t2
    ld t0, TF_USER_SP_OFF(sp)
    csrw mscratch, t0

    ld ra, TF_RA_OFF(sp)
    ld gp, TF_GP_OFF(sp)
    ld tp, TF_TP_OFF(sp)
    ld t2, TF_T2_OFF(sp)
    ld s0, TF_S0_OFF(sp)
    ld s1, TF_S1_OFF(sp)
    ld a0, TF_A0_OFF(sp)
    ld a1, TF_A1_OFF(sp)
    ld a2, TF_A2_OFF(sp)
    ld a3, TF_A3_OFF(sp)
    ld a4, TF_A4_OFF(sp)
    ld a5, TF_A5_OFF(sp)
    ld a6, TF_A6_OFF(sp)
    ld a7, TF_A7_OFF(sp)
    ld s2, TF_S2_OFF(sp)
    ld s3, TF_S3_OFF(sp)
    ld s4, TF_S4_OFF(sp)
    ld s5, TF_S5_OFF(sp)
    ld s6, TF_S6_OFF(sp)
    ld s7, TF_S7_OFF(sp)
    ld s8, TF_S8_OFF(sp)
    ld s9, TF_S9_OFF(sp)
    ld s10, TF_S10_OFF(sp)
    ld s11, TF_S11_OFF(sp)
    ld t3, TF_T3_OFF(sp)
    ld t4, TF_T4_OFF(sp)
    ld t5, TF_T5_OFF(sp)
    ld t6, TF_T6_OFF(sp)

    li t1, (3 << 11)
    csrc mstatus, t1
    li t1, (1 << 7)
    csrs mstatus, t1

    ld t0, TF_T0_OFF(sp)
    ld t1, TF_T1_OFF(sp)
    addi sp, sp, TRAPFRAME_SIZE
    csrrw sp, mscratch, sp
    mret
