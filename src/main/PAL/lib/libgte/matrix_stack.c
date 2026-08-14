#include "common.h"

/*
 * PSY-Q 3.5 libgte object mtx_00.o (LIBGTE.A) as one translation unit.
 * Exports, in link order: CompMatrix, MulMatrix0, MulRotMatrix0,
 * MulRotMatrix, SetMulMatrix, ApplyMatrixLV, ApplyRotMatrix (TransformCollisionVector),
 * ScaleMatrixL, PushMatrix, PopMatrix, ReadRotMatrix, ReadLightMatrix,
 * ReadColorMatrix.  Boundaries and names byte-matched against mtx_00.o.
 * HANDWRITTEN_ASM (asm-in-C), excluded from
 * progress; see README.md.
 */

/*
 * HANDWRITTEN_ASM - excluded from progress; see README.md.
 * CompMatrix is a hand-inlined PSY-Q libgte CompMatrix routine: batched
 * cop2 (ctc2/mtc2/mvmva/mfc2) transfers interleaved with by-hand 16-bit column
 * packing (lui/and/or/sll) that GCC 2.6.3 cannot reproduce from C or from GTE
 * macros. Same family as the already-HANDWRITTEN siblings MulMatrix0/CA4/
 * D88/E70/F80. Byte-exact via register-pinned COP2 asm.
 *
 * The func_80068A2C TU begins with 12 bytes (three 0x00000000 words) of
 * inter-object alignment padding that splat over-split into three bogus
 * single-`nop` "functions" (func_80068A2C/A30/A34).  The previous TU
 * (SquareRoot0, [0x591A8,0x5922C)) ends cleanly at 0x5922C with
 * `jr ra; li v0,0`, so these three words are leading pad, not its tail.
 * They are emitted here the same way every other pure-padding TU in this
 * binary is (compare func_80069CBC: `u32 x[3] __attribute__((section(".text")))`).
 */
u32 func_80068A2C[3] __attribute__((section(".text"))) = { 0, 0, 0 };

/*
 * CompMatrix is a verbatim hand-inlined PSY-Q libgte matrix routine
 * (CompMatrix-family: R2 = R0 * R1, T2 = R0 * T1 + T0) built from three/four
 * MVMVA (cop2 0x486012, sf=1, mx=Rot, v=V0, cv=none) commands with the column
 * vectors packed out of the source MATRIX by hand.  It belongs to the same
 * family as its neighbours MulMatrix0/CA4/D88/E70 and, like them, only
 * reaches a byte-exact match through register-pinned COP2 transfers: GCC 2.6.3
 * will not reproduce the batched 5x lw / 5x ctc2 load block nor the exact
 * IR/MAC extraction register schedule from natural C.
 */
void *CompMatrix(s32 *m0, void *m1, void *m2) {
    asm volatile(
        "lw $8,0(%0)\n"
        "lw $9,4(%0)\n"
        "lw $10,8(%0)\n"
        "lw $11,12(%0)\n"
        "lw $12,16(%0)\n"
        "ctc2 $8,$0\n"
        "ctc2 $9,$1\n"
        "ctc2 $10,$2\n"
        "ctc2 $11,$3\n"
        "ctc2 $12,$4\n"
        "lhu $8,0($5)\n"
        "lw $9,4($5)\n"
        "lw $10,12($5)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,2($5)\n"
        "lw $9,8($5)\n"
        "lh $10,14($5)\n"
        "sll $9,$9,16\n"
        "or $8,$8,$9\n"
        "mfc2 $11,$9\n"
        "mfc2 $12,$10\n"
        "mfc2 $13,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,4($5)\n"
        "lw $9,8($5)\n"
        "lw $10,16($5)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mfc2 $14,$9\n"
        "mfc2 $15,$10\n"
        "mfc2 $24,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "andi $11,$11,0xFFFF\n"
        "sll $14,$14,16\n"
        "or $14,$14,$11\n"
        "sw $14,0($6)\n"
        "andi $13,$13,0xFFFF\n"
        "sll $24,$24,16\n"
        "or $24,$24,$13\n"
        "sw $24,12($6)\n"
        "mfc2 $8,$9\n"
        "mfc2 $9,$10\n"
        "swc2 $11,16($6)\n"
        "lhu $13,20($5)\n"
        "lw $14,24($5)\n"
        "lw $10,28($5)\n"
        "sll $14,$14,16\n"
        "or $13,$13,$14\n"
        "mtc2 $13,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "sll $12,$12,16\n"
        "andi $8,$8,0xFFFF\n"
        "or $8,$8,$12\n"
        "sw $8,4($6)\n"
        "andi $15,$15,0xFFFF\n"
        "sll $9,$9,16\n"
        "or $9,$9,$15\n"
        "sw $9,8($6)\n"
        "mfc2 $8,$25\n"
        "mfc2 $9,$26\n"
        "mfc2 $10,$27\n"
        "lw $11,20($4)\n"
        "lw $12,24($4)\n"
        "lw $13,28($4)\n"
        "add $8,$8,$11\n"
        "add $9,$9,$12\n"
        "add $10,$10,$13\n"
        "sw $8,20($6)\n"
        "sw $9,24($6)\n"
        "sw $10,28($6)"
        :
        : "r"(m0), "r"(m1), "r"(m2)
        );
    asm volatile("move $2,$6");
}


/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


void *MulMatrix0(s32 *matrix, void *src, void *dst) {
    asm volatile(
        "lw $8,0(%0)\n"
        "lw $9,4(%0)\n"
        "lw $10,8(%0)\n"
        "lw $11,12(%0)\n"
        "lw $12,16(%0)\n"
        "ctc2 $8,$0\n"
        "ctc2 $9,$1\n"
        "ctc2 $10,$2\n"
        "ctc2 $11,$3\n"
        "ctc2 $12,$4\n"
        "lhu $8,0($5)\n"
        "lw $9,4($5)\n"
        "lw $10,12($5)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,2($5)\n"
        "lw $9,8($5)\n"
        "lh $10,14($5)\n"
        "sll $9,$9,16\n"
        "or $8,$8,$9\n"
        "mfc2 $11,$9\n"
        "mfc2 $12,$10\n"
        "mfc2 $13,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,4($5)\n"
        "lw $9,8($5)\n"
        "lw $10,16($5)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mfc2 $14,$9\n"
        "mfc2 $15,$10\n"
        "mfc2 $24,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "andi $11,$11,0xFFFF\n"
        "sll $14,$14,16\n"
        "or $14,$14,$11\n"
        "sw $14,0($6)\n"
        "andi $13,$13,0xFFFF\n"
        "sll $24,$24,16\n"
        "or $24,$24,$13\n"
        "sw $24,12($6)\n"
        "mfc2 $8,$9\n"
        "mfc2 $9,$10\n"
        "andi $8,$8,0xFFFF\n"
        "sll $12,$12,16\n"
        "or $8,$8,$12\n"
        "sw $8,4($6)\n"
        "andi $15,$15,0xFFFF\n"
        "sll $9,$9,16\n"
        "or $9,$9,$15\n"
        "sw $9,8($6)\n"
        "swc2 $11,16($6)"
        :
        : "r"(matrix), "r"(src), "r"(dst)
        );
    asm volatile("move $2,$6");
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


void *MulRotMatrix0(void *lhs, void *rhs) {
    asm volatile(
        "lhu $8,0($4)\n"
        "lw $9,4($4)\n"
        "lw $10,12($4)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,2($4)\n"
        "lw $9,8($4)\n"
        "lh $10,14($4)\n"
        "sll $9,$9,16\n"
        "or $8,$8,$9\n"
        "mfc2 $11,$9\n"
        "mfc2 $12,$10\n"
        "mfc2 $13,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,4($4)\n"
        "lw $9,8($4)\n"
        "lw $10,16($4)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mfc2 $14,$9\n"
        "mfc2 $15,$10\n"
        "mfc2 $24,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "andi $11,$11,0xFFFF\n"
        "sll $14,$14,16\n"
        "or $14,$14,$11\n"
        "sw $14,0($5)\n"
        "andi $13,$13,0xFFFF\n"
        "sll $24,$24,16\n"
        "or $24,$24,$13\n"
        "sw $24,12($5)\n"
        "mfc2 $8,$9\n"
        "mfc2 $9,$10\n"
        "andi $8,$8,0xFFFF\n"
        "sll $12,$12,16\n"
        "or $8,$8,$12\n"
        "sw $8,4($5)\n"
        "andi $15,$15,0xFFFF\n"
        "sll $9,$9,16\n"
        "or $9,$9,$15\n"
        "sw $9,8($5)\n"
        "swc2 $11,16($5)"
        :
        : "r"(lhs), "r"(rhs)
        );
    asm volatile("move $2,$5");
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


void *MulRotMatrix(void *mtx) {
    asm volatile(
        "lw $8,0($4)\n"
        "lw $9,4($4)\n"
        "lw $10,12($4)\n"
        "andi $8,$8,0xFFFF\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,2($4)\n"
        "lw $9,8($4)\n"
        "lh $10,14($4)\n"
        "sll $9,$9,16\n"
        "or $8,$8,$9\n"
        "mfc2 $11,$9\n"
        "mfc2 $12,$10\n"
        "mfc2 $13,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,4($4)\n"
        "lw $9,8($4)\n"
        "lw $10,16($4)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mfc2 $14,$9\n"
        "mfc2 $15,$10\n"
        "mfc2 $24,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "andi $11,$11,0xFFFF\n"
        "sll $14,$14,16\n"
        "or $14,$14,$11\n"
        "sw $14,0($4)\n"
        "andi $13,$13,0xFFFF\n"
        "sll $24,$24,16\n"
        "or $24,$24,$13\n"
        "sw $24,12($4)\n"
        "mfc2 $8,$9\n"
        "mfc2 $9,$10\n"
        "andi $8,$8,0xFFFF\n"
        "sll $12,$12,16\n"
        "or $8,$8,$12\n"
        "sw $8,4($4)\n"
        "andi $15,$15,0xFFFF\n"
        "sll $9,$9,16\n"
        "or $9,$9,$15\n"
        "sw $9,8($4)\n"
        "swc2 $11,16($4)"
        :
        : "r"(mtx)
        );
    asm volatile("move $2,$4");
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


void *SetMulMatrix(s32 *matrix, void *src) {
    asm volatile(
        "lw $8,0(%0)\n"
        "lw $9,4(%0)\n"
        "lw $10,8(%0)\n"
        "lw $11,12(%0)\n"
        "lw $12,16(%0)\n"
        "ctc2 $8,$0\n"
        "ctc2 $9,$1\n"
        "ctc2 $10,$2\n"
        "ctc2 $11,$3\n"
        "ctc2 $12,$4\n"
        "lhu $8,0($5)\n"
        "lw $9,4($5)\n"
        "lw $10,12($5)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,2($5)\n"
        "lw $9,8($5)\n"
        "lh $10,14($5)\n"
        "sll $9,$9,16\n"
        "or $8,$8,$9\n"
        "mfc2 $11,$9\n"
        "mfc2 $12,$10\n"
        "mfc2 $13,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "lhu $8,4($5)\n"
        "lw $9,8($5)\n"
        "lw $10,16($5)\n"
        "lui $1,0xFFFF\n"
        "and $9,$9,$1\n"
        "or $8,$8,$9\n"
        "mfc2 $14,$9\n"
        "mfc2 $15,$10\n"
        "mfc2 $24,$11\n"
        "mtc2 $8,$0\n"
        "mtc2 $10,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "andi $11,$11,0xFFFF\n"
        "sll $14,$14,16\n"
        "or $14,$14,$11\n"
        "andi $13,$13,0xFFFF\n"
        "sll $24,$24,16\n"
        "or $24,$24,$13\n"
        "mfc2 $8,$9\n"
        "mfc2 $9,$10\n"
        "mfc2 $10,$11\n"
        "andi $8,$8,0xFFFF\n"
        "sll $12,$12,16\n"
        "or $8,$8,$12\n"
        "andi $15,$15,0xFFFF\n"
        "sll $9,$9,16\n"
        "or $9,$9,$15\n"
        "ctc2 $14,$0\n"
        "ctc2 $8,$1\n"
        "ctc2 $9,$2\n"
        "ctc2 $24,$3\n"
        "ctc2 $10,$4"
        :
        : "r"(matrix), "r"(src)
        );
    asm volatile("move $2,$4");
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


void *ApplyMatrixLV(void *mtx, void *vec, void *out) {
    void *m = mtx;
    void *v = vec;
    register void *o asm("$6") = out;

    asm volatile(
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "lw $8,0($4)\n"
        "lw $9,4($4)\n"
        "lw $10,8($4)\n"
        "lw $11,12($4)\n"
        "lw $12,16($4)\n"
        "ctc2 $8,$0\n"
        "ctc2 $9,$1\n"
        "ctc2 $10,$2\n"
        "ctc2 $11,$3\n"
        "ctc2 $12,$4\n"
        "lw $8,0($5)\n"
        "lw $9,4($5)\n"
        "lw $10,8($5)\n"
        "bgez $8,1f\n"
        "sra $11,$8,0xf\n"
        "negu $8,$8\n"
        "sra $11,$8,0xf\n"
        "andi $8,$8,0x7fff\n"
        "negu $11,$11\n"
        "b 2f\n"
        "negu $8,$8\n"
        "sra $11,$8,0xf\n"
        "1:\n"
        "andi $8,$8,0x7fff\n"
        "2:\n"
        "bgez $9,3f\n"
        "sra $12,$9,0xf\n"
        "negu $9,$9\n"
        "sra $12,$9,0xf\n"
        "andi $9,$9,0x7fff\n"
        "negu $12,$12\n"
        "b 4f\n"
        "negu $9,$9\n"
        "sra $12,$9,0xf\n"
        "3:\n"
        "andi $9,$9,0x7fff\n"
        "4:\n"
        "bgez $10,5f\n"
        "sra $13,$10,0xf\n"
        "negu $10,$10\n"
        "sra $13,$10,0xf\n"
        "andi $10,$10,0x7fff\n"
        "negu $13,$13\n"
        "b 6f\n"
        "negu $10,$10\n"
        "sra $13,$10,0xf\n"
        "5:\n"
        "andi $10,$10,0x7fff\n"
        "6:\n"
        "mtc2 $11,$9\n"
        "mtc2 $12,$10\n"
        "mtc2 $13,$11\n"
        "nop\n"
        "cop2 0x41e012\n"
        "mfc2 $11,$25\n"
        "mfc2 $12,$26\n"
        "mfc2 $13,$27\n"
        "mtc2 $8,$9\n"
        "mtc2 $9,$10\n"
        "mtc2 $10,$11\n"
        "nop\n"
        "cop2 0x49e012\n"
        "bgez $11,7f\n"
        "nop\n"
        "negu $11,$11\n"
        "sll $11,$11,0x3\n"
        "b 8f\n"
        "negu $11,$11\n"
        "7:\n"
        "sll $11,$11,0x3\n"
        "8:\n"
        "bgez $12,9f\n"
        "nop\n"
        "negu $12,$12\n"
        "sll $12,$12,0x3\n"
        "b 10f\n"
        "negu $12,$12\n"
        "9:\n"
        "sll $12,$12,0x3\n"
        "10:\n"
        "bgez $13,11f\n"
        "nop\n"
        "negu $13,$13\n"
        "sll $13,$13,0x3\n"
        "b 12f\n"
        "negu $13,$13\n"
        "11:\n"
        "sll $13,$13,0x3\n"
        "12:\n"
        "mfc2 $8,$25\n"
        "mfc2 $9,$26\n"
        "mfc2 $10,$27\n"
        "addu $8,$8,$11\n"
        "addu $9,$9,$12\n"
        "addu $10,$10,$13\n"
        "sw $8,0($6)\n"
        "sw $9,4($6)\n"
        "sw $10,8($6)\n"
        ".set\treorder\n"
        :
        : "r"(m), "r"(v), "r"(o)
        : "$8", "$9", "$10", "$11", "$12", "$13");
    return o;
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


s32 TransformCollisionVector(s32 *in, s32 *out, s32 flag) {
    asm volatile(
        "lw $8,0(%0)\n"
        "lw $9,4(%0)\n"
        "mtc2 $8,$0\n"
        "mtc2 $9,$1\n"
        "nop\n"
        "cop2 0x486012\n"
        "swc2 $9,0($5)\n"
        "swc2 $10,4($5)\n"
        "swc2 $11,8($5)"
        :
        : "r"(in), "r"(out)
        );
    asm volatile("move $2,$6");
}

/*
 * HANDWRITTEN_ASM - excluded from progress; see README.md.
 *
 * Symbol:   func_80069110 = ScaleMatrixL (PSY-Q libgte; see include/psyq/gte.h).
 *           m[i][j] *= v[i], the row-scaling twin of ScaleMatrix
 *           (func_80069728) - Sony's Run-Time Library Reference, 8-151.
 *           Unreferenced in this image.
 * Reason:   hand-written libgte SDK assembly, not compiler C.
 * Evidence: every fixed-point product uses a NARROW unsigned multiply
 *           (multu + mflo, no mfhi) - the asm author's idiom. GCC 2.6.3 AND
 *           2.7.2 (this repo's cc1) canonicalise every truncated 32-bit
 *           multiply to signed mult; narrow multu is unreachable from C, and
 *           NO matched function in the project emits it. It also folds
 *           (s16)word / word>>16 into lh loads vs the retail's single-lw +
 *           andi/sll/sra extraction. Sibling RotMatrix = RotMatrix (14x
 *           multu), a documented PSY-Q libgte asm routine.
 * Revisit:  only if the exact gcc-2.7.2 variant that emits narrow multu is
 *           obtained AND verified not to regress already-matched functions.
 */

/* PushMatrix and PopMatrix: GTE control registers moved to and from the
 * matrix stack, hand-written in the original. Excluded from progress.
 * See src/main/PAL/lib/libgte/matrix_stack.s. */
HANDWRITTEN_ASM("src/main/PAL/lib/libgte", matrix_stack);

/* Read GTE rotation matrix + translation (control regs $0..$7) into p[0..7]. */
void ReadRotMatrix(volatile u32 *p) {
    asm volatile(
        "cfc2 $8,$0\n"
        "cfc2 $9,$1\n"
        "cfc2 $10,$2\n"
        "cfc2 $11,$3\n"
        "cfc2 $12,$4\n"
        "sw $8,0(%0)\n"
        "sw $9,4(%0)\n"
        "sw $10,8(%0)\n"
        "sw $11,12(%0)\n"
        "sw $12,16(%0)\n"
        "cfc2 $8,$5\n"
        "cfc2 $9,$6\n"
        "cfc2 $10,$7\n"
        "sw $8,20(%0)\n"
        "sw $9,24(%0)\n"
        "sw $10,28(%0)"
        :
        : "r"(p)
        : "$8", "$9", "$10", "$11", "$12");
}

/* Read GTE light matrix + back-color (control regs $8..$15) into p[0..7]. */
void ReadLightMatrix(volatile u32 *p) {
    asm volatile(
        "cfc2 $8,$8\n"
        "cfc2 $9,$9\n"
        "cfc2 $10,$10\n"
        "cfc2 $11,$11\n"
        "cfc2 $12,$12\n"
        "sw $8,0(%0)\n"
        "sw $9,4(%0)\n"
        "sw $10,8(%0)\n"
        "sw $11,12(%0)\n"
        "sw $12,16(%0)\n"
        "cfc2 $8,$13\n"
        "cfc2 $9,$14\n"
        "cfc2 $10,$15\n"
        "sw $8,20(%0)\n"
        "sw $9,24(%0)\n"
        "sw $10,28(%0)"
        :
        : "r"(p)
        : "$8", "$9", "$10", "$11", "$12");
}

/* Read GTE color matrix + far-color (control regs $16..$23) into p[0..7]. */
void ReadColorMatrix(volatile u32 *p) {
    asm volatile(
        "cfc2 $8,$16\n"
        "cfc2 $9,$17\n"
        "cfc2 $10,$18\n"
        "cfc2 $11,$19\n"
        "cfc2 $12,$20\n"
        "sw $8,0(%0)\n"
        "sw $9,4(%0)\n"
        "sw $10,8(%0)\n"
        "sw $11,12(%0)\n"
        "sw $12,16(%0)\n"
        "cfc2 $8,$21\n"
        "cfc2 $9,$22\n"
        "cfc2 $10,$23\n"
        "sw $8,20(%0)\n"
        "sw $9,24(%0)\n"
        "sw $10,28(%0)"
        :
        : "r"(p)
        : "$8", "$9", "$10", "$11", "$12");
}

/* trailing alignment padding (3 nops) that fills this TU to the next subsegment */
const u32 func_8006944C[3] __attribute__((section(".text"))) = { 0, 0, 0 };
