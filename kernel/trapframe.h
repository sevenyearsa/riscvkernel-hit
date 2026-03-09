#ifndef TRAPFRAME_H
#define TRAPFRAME_H

#ifndef __ASSEMBLER__
#include "stdint.h"

struct trapframe {
    uint64_t epc;
    uint64_t ra;
    uint64_t user_sp;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t s0;
    uint64_t s1;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
    uint64_t t3;
    uint64_t t4;
    uint64_t t5;
    uint64_t t6;
};

static inline void trapframe_clear(struct trapframe *tf) {
    uint64_t *words = (uint64_t *)tf;

    for (int i = 0; i < (int)(sizeof(*tf) / sizeof(uint64_t)); i++) {
        words[i] = 0;
    }
}

static inline void trapframe_copy(struct trapframe *dst, const struct trapframe *src) {
    const uint64_t *src_words = (const uint64_t *)src;
    uint64_t *dst_words = (uint64_t *)dst;

    for (int i = 0; i < (int)(sizeof(*dst) / sizeof(uint64_t)); i++) {
        dst_words[i] = src_words[i];
    }
}
#endif

#define TRAPFRAME_WORDS 32
#define TRAPFRAME_SIZE (TRAPFRAME_WORDS * 8)

#define TF_EPC_OFF      (0 * 8)
#define TF_RA_OFF       (1 * 8)
#define TF_USER_SP_OFF  (2 * 8)
#define TF_GP_OFF       (3 * 8)
#define TF_TP_OFF       (4 * 8)
#define TF_T0_OFF       (5 * 8)
#define TF_T1_OFF       (6 * 8)
#define TF_T2_OFF       (7 * 8)
#define TF_S0_OFF       (8 * 8)
#define TF_S1_OFF       (9 * 8)
#define TF_A0_OFF       (10 * 8)
#define TF_A1_OFF       (11 * 8)
#define TF_A2_OFF       (12 * 8)
#define TF_A3_OFF       (13 * 8)
#define TF_A4_OFF       (14 * 8)
#define TF_A5_OFF       (15 * 8)
#define TF_A6_OFF       (16 * 8)
#define TF_A7_OFF       (17 * 8)
#define TF_S2_OFF       (18 * 8)
#define TF_S3_OFF       (19 * 8)
#define TF_S4_OFF       (20 * 8)
#define TF_S5_OFF       (21 * 8)
#define TF_S6_OFF       (22 * 8)
#define TF_S7_OFF       (23 * 8)
#define TF_S8_OFF       (24 * 8)
#define TF_S9_OFF       (25 * 8)
#define TF_S10_OFF      (26 * 8)
#define TF_S11_OFF      (27 * 8)
#define TF_T3_OFF       (28 * 8)
#define TF_T4_OFF       (29 * 8)
#define TF_T5_OFF       (30 * 8)
#define TF_T6_OFF       (31 * 8)

#endif
