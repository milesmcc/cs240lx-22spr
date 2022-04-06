#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__
// engler, cs240lx: simplistic instruction encodings for r/pi ARMV6.
// this will compile both on our bare-metal r/pi and unix.

// bit better type checking to use enums.
enum {
    arm_r0 = 0, 
    arm_r1, 
    arm_r2,
    arm_r3,
    arm_r4,
    arm_r5,
    arm_r6,
    arm_r7,
    arm_r8,
    arm_r9,
    arm_r10,
    arm_r11,
    arm_r12,
    arm_r13,
    arm_r14,
    arm_r15,
    arm_sp = arm_r13,
    arm_lr = arm_r14,
    arm_pc = arm_r15
};
_Static_assert(arm_r15 == 15, "bad enum");


// condition code.
enum {
    arm_EQ = 0,
    arm_NE,
    arm_CS,
    arm_CC,
    arm_MI,
    arm_PL,
    arm_VS,
    arm_VC,
    arm_HI,
    arm_LS,
    arm_GE,
    arm_LT,
    arm_GT,
    arm_LE,
    arm_AL,
};
_Static_assert(arm_AL == 0b1110, "bad enum list");

// data processing ops.
enum {
    arm_and_op = 0, 
    arm_eor_op,
    arm_sub_op,
    arm_rsb_op,
    arm_add_op,
    arm_adc_op,
    arm_sbc_op,
    arm_rsc_op,
    arm_tst_op,
    arm_teq_op,
    arm_cmp_op,
    arm_cmn_op,
    arm_orr_op,
    arm_mov_op,
    arm_bic_op,
    arm_mvn_op,
};
_Static_assert(arm_mvn_op == 0b1111, "bad num list");

/************************************************************
 * instruction encodings.  should do:
 *      bx lr
 *      ld *
 *      st *
 *      cmp
 *      branch
 *      alu 
 *      alu_imm
 *      jump
 *      call
 */

// add instruction:
//      add rdst, rs1, rs2
//  - general add instruction: page A4-6 [armv6.pdf]
//  - shift operatnd: page A5-8 [armv6.pdf]
//
// we do not do any carries, so S = 0.


static int arm_add(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0800000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}

// <add> of an immediate
static inline uint32_t arm_add_imm8(uint8_t rd, uint8_t rs1, uint8_t imm) {
    return 0xe2800000 | (rd << 12) | (rs1 << 16) | (imm << 0);
}

static inline uint32_t arm_bx(uint32_t dst) {
    return 0xe12fff10 | (dst << 0);
}

static inline uint32_t arm_ldr(uint32_t dst, uint32_t loc, uint32_t offset) {
    return 0xe5900000 | (dst << 12) | (loc << 16) | ((offset << 0) & 0xfff);
}

// load an or immediate and rotate it.
static inline uint32_t 
arm_or_imm_rot(uint8_t rd, uint8_t rs1, uint8_t imm8, uint8_t rot_nbits) {
    unimplemented();
}

static inline uint32_t arm_bl(uint32_t from, uint32_t to) {
    int32_t delta = (to - (from + 8)) / 4;
    return 0xeb000000 | ((delta) & (0xffffff));
}

static inline uint32_t arm_b(uint32_t from, uint32_t to) {
    int32_t delta = (to - (from + 8)) / 4;
    return 0xea000000 | ((delta) & (0xffffff));
}

static inline uint32_t arm_push(uint32_t reg) {
    return 0xe52d0004 | (reg << 12);
}

static inline uint32_t arm_pop(uint32_t reg) {
    return 0xe49d0004 | (reg << 12);
}

static inline uint32_t arm_mov(uint32_t dst, uint32_t src) {
    return 0xe1a00000 | (dst << 12) | (src << 0);
}

static inline uint32_t arm_mov_imm(uint32_t dst, uint32_t val) {
    return 0xe3a00000 | (dst << 12) | (val << 0);
}

#endif
