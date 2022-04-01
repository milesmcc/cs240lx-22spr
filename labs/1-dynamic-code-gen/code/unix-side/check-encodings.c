#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include "libunix.h"
#include <unistd.h>
#include "code-gen.h"
#include "armv6-insts.h"

/*
 *  1. emits <insts> into a temporary file.
 *  2. compiles it.
 *  3. reads back in.
 *  4. returns pointer to it.
 */
uint32_t *insts_emit(unsigned *nbytes, char *insts) {
    // check libunix.h --- create_file, write_exact, run_system, read_file.
    int fd = create_file("_temp.s");
    write_exact(fd, insts, strlen(insts));
    run_system("echo >> _temp.s");
    run_system("arm-none-eabi-as -mcpu=arm1176jzf-s -march=armv6zk _temp.s -o _temp.o");
    run_system("arm-none-eabi-objcopy _temp.o -O binary _temp.bin");
    return (uint32_t *) read_file(nbytes, "_temp.bin");
}

/*
 * a cross-checking hack that uses the native GNU compiler/assembler to 
 * check our instruction encodings.
 *  1. compiles <insts>
 *  2. compares <code,nbytes> to it for equivalance.
 *  3. prints out a useful error message if it did not succeed!!
 */
void insts_check(char *insts, uint32_t *code, unsigned nbytes) {
    // make sure you print out something useful on mismatch!
    unsigned emitted_nbytes;
    uint32_t *data = insts_emit(&emitted_nbytes, insts);

    for(unsigned i = 0; i < nbytes / 4; i++) {
        if (data[i] != code[i]) {
            printf("[mismatch @ %x] given=%x, compiled=%x (inst #%d)\n", i * 4, code[i], data[i], i);
        } else {
            // printf("[match @ %x] given=%x, compiled=%x (inst #%d)\n", i * 4, code[i], data[i], i);
        }
    }
}

// check a single instruction.
void check_one_inst(char *insts, uint32_t inst) {
    return insts_check(insts, &inst, 4);
}

// helper function to make reverse engineering instructions a bit easier.
void insts_print(char *insts) {
    // emit <insts>
    unsigned gen_nbytes;
    uint32_t *gen = insts_emit(&gen_nbytes, insts);

    // print the result.
    output("getting encoding for: < %20s >\t= [", insts);
    unsigned n = gen_nbytes / 4;
    for(int i = 0; i < n; i++)
         output(" 0x%x ", gen[i]);
    output("]\n");
}


// helper function for reverse engineering.  you should refactor its interface
// so your code is better.
uint32_t emit_rrr(const char *op, const char *d, const char *s1, const char *s2) {
    char buf[1024];
    sprintf(buf, "%s %s, %s, %s", op, d, s1, s2);

    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}

// helper function for reverse engineering.  you should refactor its interface
// so your code is better.
uint32_t emit_rrrr(const char *op, const char *d, const char *s1, const char *s2, const char *s3) {
    char buf[1024];
    sprintf(buf, "%s %s, %s, %s, %s", op, d, s1, s2, s3);

    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}

// overly-specific.  some assumptions:
//  1. each register is encoded with its number (r0 = 0, r1 = 1)
//  2. related: all register fields are contiguous.
//
// NOTE: you should probably change this so you can emit all instructions 
// all at once, read in, and then solve all at once.
//
// For lab:
//  1. complete this code so that it solves for the other registers.
//  2. refactor so that you can reused the solving code vs cut and pasting it.
//  3. extend system_* so that it returns an error.
//  4. emit code to check that the derived encoding is correct.
//  5. emit if statements to checks for illegal registers (those not in <src1>,
//    <src2>, <dst>).
void derive_op_rrr(const char *name, const char *opcode, 
        const char **dst, const char **src1, const char **src2) {

    const char *s1 = src1[0];
    const char *s2 = src2[0];
    const char *d = dst[0];
    assert(d && s1 && s2);

    unsigned d_off = 0, src1_off = 0, src2_off = 0, op = ~0;

    uint8_t offsets[4];
    uint32_t changed_full = 0;

    for(int idx = 0; idx < 4; idx++) {
        uint32_t always_0 = ~0, always_1 = ~0;

        // Hack
        const char **vals;
        if (idx == 0) { vals = dst; };
        if (idx == 1) { vals = src1; };
        if (idx == 2) { vals = src2; };

        // compute any bits that changed as we vary d.
        for(unsigned i = 0; vals[i]; i++) {
            uint32_t u;
            if (idx == 0) u = emit_rrr(opcode, vals[i], s1, s2);
            if (idx == 1) u = emit_rrr(opcode, d, vals[i], s2);
            if (idx == 2) u = emit_rrr(opcode, d, s1, vals[i]);

            // if a bit is always 0 then it will be 1 in always_0
            always_0 &= ~u;

            // if a bit is always 1 it will be 1 in always_1, otherwise 0
            always_1 &= u;
        }

        if(always_0 & always_1) 
            panic("impossible overlap: always_0 = %x, always_1 %x\n", 
                always_0, always_1);

        // bits that never changed
        uint32_t never_changed = always_0 | always_1;
        // bits that changed: these are the register bits.
        uint32_t changed = ~never_changed;

        changed_full |= changed;

        // find the offset.  we assume register bits are contig and within 0xf
        unsigned off = ffs(changed);
        if (idx == 0) { d_off = off - 1; };
        if (idx == 1) { src1_off = off - 1; };
        if (idx == 2) { src2_off = off - 1; };
        
        // check that bits are contig and at most 4 bits are set.
        // if(((changed >> off) & ~0xf) != 0)
        //     panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);
    }

    // refine the opcode.
    op &= ~(changed_full);
    op &= emit_rrr(opcode, d, s1, s2);

    output("opcode is in =%x\n", op);

    // emit: NOTE: obviously, currently <src1_off>, <src2_off> are not 
    // defined (so solve for them) and opcode needs to be refined more.
    output("static int %s(uint32_t dst, uint32_t src1, uint32_t src2) {\n", name);
    output("    return 0x%x | (dst << %d) | (src1 << %d) | (src2 << %d);\n",
                op,
                d_off,
                src1_off,
                src2_off);
    output("}\n");
}

/*
 * 1. we start by using the compiler / assembler tool chain to get / check
 *    instruction encodings.  this is sleazy and low-rent.   however, it 
 *    lets us get quick and dirty results, removing a bunch of the mystery.
 *
 * 2. after doing so we encode things "the right way" by using the armv6
 *    manual (esp chapters a3,a4,a5).  this lets you see how things are 
 *    put together.  but it is tedious.
 *
 * 3. to side-step tedium we use a variant of (1) to reverse engineer 
 *    the result.
 *
 *    we are only doing a small number of instructions today to get checked off
 *    (you, of course, are more than welcome to do a very thorough set) and focus
 *    on getting each method running from beginning to end.
 *
 * 4. then extend to a more thorough set of instructions: branches, loading
 *    a 32-bit constant, function calls.
 *
 * 5. use (4) to make a simple object oriented interface setup.
 *    you'll need: 
 *      - loads of 32-bit immediates
 *      - able to push onto a stack.
 *      - able to do a non-linking function call.
 */
int main(void) {
    // part 1: implement the code to do this.
    output("-----------------------------------------\n");
    output("part1: checking: correctly generating assembly.\n");
    insts_print("add r0, r0, r1");
    insts_print("bx lr");
    insts_print("mov r0, #1");
    insts_print("nop");
    insts_print("add r0, r1, r2");
    insts_print("add r3, r4, r5");
    output("\n");
    output("success!\n");

    // part 2: implement the code so these checks pass.
    // these should all pass.
    output("\n-----------------------------------------\n");
    output("part 2: checking we correctly compare asm to machine code.\n");
    check_one_inst("add r0, r0, r1", 0xe0800001);
    check_one_inst("bx lr", 0xe12fff1e);
    check_one_inst("mov r0, #1", 0xe3a00001);
    check_one_inst("nop", 0xe320f000);
    output("success!\n");

    // part 3: check that you can correctly encode an add instruction.
    output("\n-----------------------------------------\n");
    output("part3: checking that we can generate an <add> by hand\n");
    check_one_inst("add r0, r1, r2", arm_add(arm_r0, arm_r1, arm_r2));
    check_one_inst("add r3, r4, r5", arm_add(arm_r3, arm_r4, arm_r5));
    check_one_inst("add r6, r7, r8", arm_add(arm_r6, arm_r7, arm_r8));
    check_one_inst("add r9, r10, r11", arm_add(arm_r9, arm_r10, arm_r11));
    check_one_inst("add r12, r13, r14", arm_add(arm_r12, arm_r13, arm_r14));
    check_one_inst("add r15, r7, r3", arm_add(arm_r15, arm_r7, arm_r3));
    check_one_inst("add r0, r1, r2", arm_add_gen(arm_r0, arm_r1, arm_r2));
    check_one_inst("add r3, r4, r5", arm_add_gen(arm_r3, arm_r4, arm_r5));
    check_one_inst("add r6, r7, r8", arm_add_gen(arm_r6, arm_r7, arm_r8));
    check_one_inst("add r9, r10, r11", arm_add_gen(arm_r9, arm_r10, arm_r11));
    check_one_inst("add r12, r13, r14", arm_add_gen(arm_r12, arm_r13, arm_r14));
    check_one_inst("add r15, r7, r3", arm_add_gen(arm_r15, arm_r7, arm_r3));
    output("success!\n");

    // part 4: implement the code so it will derive the add instruction.
    output("\n-----------------------------------------\n");
    output("part4: checking that we can reverse engineer an <add>\n");

    const char *all_regs[] = {
                "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                0 
    };
    const char *my_imms[] = {"#0", "#1", "#7", "#8", "#9", "#15", "#16", "#17", "#255", 0};
    const char *my_empty[] = {"\0"};

    // XXX: should probably pass a bitmask in instead.
    derive_op_rrr("arm_add", "add", all_regs, all_regs, all_regs);
    
    derive_op_rrr("arm_add_imm8", "add", all_regs, all_regs, my_imms);
    derive_op_rrr("add_or_imm_rot", "bx", all_regs, my_empty, my_empty);
    output("did something: now use the generated code in the checks above!\n");

    // get encodings for other instructions, loads, stores, branches, etc.
    return 0;
}
