#include "common.h"

/*
 * libgte ApplyMatrix / ApplyMatrixSV (ApplyMatrix / ApplyMatrixSV).
 * v1 = m * v0 through MVMVA; ApplyMatrix stores a LONG vector, ApplyMatrixSV a
 * SHORT vector. Not byte-matchable to a named PSY-Q 3.5 object symbol, so this
 * TU keeps its descriptive name rather than an mtx_NN.o label.
 * HANDWRITTEN_ASM - excluded from progress; see README.md.
 */

/* libgte ApplyMatrixSV: SVECTOR in, SVECTOR out, returns out. */

s32 *ApplyMatrix(s32 *matrix, s32 *vec, s32 *out) {
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
        "lwc2 $0,0($5)\n"
        "lwc2 $1,4($5)\n"
        "nop\n"
        "cop2 0x486012\n"
        "swc2 $25,0($6)\n"
        "swc2 $26,4($6)\n"
        "swc2 $27,8($6)"
        :
        : "r"(matrix), "r"(vec), "r"(out)
        );
    asm volatile("move $2,$6");
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */


s16 *ApplyMatrixSV(s32 *matrix, void *vec, s16 *out) {
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
        "lwc2 $0,0($5)\n"
        "lwc2 $1,4($5)\n"
        "nop\n"
        "cop2 0x486012\n"
        "mfc2 $8,$9\n"
        "mfc2 $9,$10\n"
        "mfc2 $10,$11\n"
        "sh $8,0($6)\n"
        "sh $9,2($6)\n"
        "sh $10,4($6)"
        :
        : "r"(matrix), "r"(vec), "r"(out)
        );
    asm volatile("move $2,$6");
}

u32 func_80069724 __attribute__((section(".text"))) = 0;
