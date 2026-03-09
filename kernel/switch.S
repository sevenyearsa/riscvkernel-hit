# switch.S
#include "context.h"
.section .text
.global switch_to

# void switch_to(struct context *prev, struct context *next);
switch_to:
    csrr t0, satp
    sd ra,  CONTEXT_RA_OFF(a0)
    sd sp,  CONTEXT_SP_OFF(a0)
    sd s0,  CONTEXT_S0_OFF(a0)
    sd s1,  CONTEXT_S1_OFF(a0)
    sd s2,  CONTEXT_S2_OFF(a0)
    sd s3,  CONTEXT_S3_OFF(a0)
    sd s4,  CONTEXT_S4_OFF(a0)
    sd s5,  CONTEXT_S5_OFF(a0)
    sd s6,  CONTEXT_S6_OFF(a0)
    sd s7,  CONTEXT_S7_OFF(a0)
    sd s8,  CONTEXT_S8_OFF(a0)
    sd s9,  CONTEXT_S9_OFF(a0)
    sd s10, CONTEXT_S10_OFF(a0)
    sd s11, CONTEXT_S11_OFF(a0)
    sd t0,  CONTEXT_SATP_OFF(a0)

    ld t0,  CONTEXT_SATP_OFF(a1)
    csrw satp, t0
    sfence.vma zero, zero
    ld ra,  CONTEXT_RA_OFF(a1)
    ld sp,  CONTEXT_SP_OFF(a1)
    ld s0,  CONTEXT_S0_OFF(a1)
    ld s1,  CONTEXT_S1_OFF(a1)
    ld s2,  CONTEXT_S2_OFF(a1)
    ld s3,  CONTEXT_S3_OFF(a1)
    ld s4,  CONTEXT_S4_OFF(a1)
    ld s5,  CONTEXT_S5_OFF(a1)
    ld s6,  CONTEXT_S6_OFF(a1)
    ld s7,  CONTEXT_S7_OFF(a1)
    ld s8,  CONTEXT_S8_OFF(a1)
    ld s9,  CONTEXT_S9_OFF(a1)
    ld s10, CONTEXT_S10_OFF(a1)
    ld s11, CONTEXT_S11_OFF(a1)
    ret
