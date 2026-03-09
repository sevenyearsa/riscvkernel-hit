#ifndef CONTEXT_H
#define CONTEXT_H

#ifndef __ASSEMBLER__
struct context {
    unsigned long ra;
    unsigned long sp;
    unsigned long s0, s1, s2, s3;
    unsigned long s4, s5, s6, s7;
    unsigned long s8, s9, s10, s11;
    unsigned long satp;
};

static inline void context_copy(struct context *dst, const struct context *src) {
    dst->ra = src->ra;
    dst->sp = src->sp;
    dst->s0 = src->s0;
    dst->s1 = src->s1;
    dst->s2 = src->s2;
    dst->s3 = src->s3;
    dst->s4 = src->s4;
    dst->s5 = src->s5;
    dst->s6 = src->s6;
    dst->s7 = src->s7;
    dst->s8 = src->s8;
    dst->s9 = src->s9;
    dst->s10 = src->s10;
    dst->s11 = src->s11;
    dst->satp = src->satp;
}
#endif

#define CONTEXT_RA_OFF    (0 * 8)
#define CONTEXT_SP_OFF    (1 * 8)
#define CONTEXT_S0_OFF    (2 * 8)
#define CONTEXT_S1_OFF    (3 * 8)
#define CONTEXT_S2_OFF    (4 * 8)
#define CONTEXT_S3_OFF    (5 * 8)
#define CONTEXT_S4_OFF    (6 * 8)
#define CONTEXT_S5_OFF    (7 * 8)
#define CONTEXT_S6_OFF    (8 * 8)
#define CONTEXT_S7_OFF    (9 * 8)
#define CONTEXT_S8_OFF    (10 * 8)
#define CONTEXT_S9_OFF    (11 * 8)
#define CONTEXT_S10_OFF   (12 * 8)
#define CONTEXT_S11_OFF   (13 * 8)
#define CONTEXT_SATP_OFF  (14 * 8)

#endif
