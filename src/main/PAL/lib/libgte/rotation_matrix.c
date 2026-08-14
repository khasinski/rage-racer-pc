#include "common.h"
#include "psyq/gte.h"

u32 func_80069CBC[3] __attribute__((section(".text"))) = { 0, 0, 0 };

/*
 * 3x3 s16 matrix transpose: dst[i][j] = src[j][i].
 * Reads src words 0,3,6,1,4,7,2,5,8; writes dst 0..8; returns dst.
 *
 * Two empty allocation barriers (no ordinary-MIPS inline asm) reproduce the
 * retail allocation/scheduling exactly:
 *   - dstp -> $5 (a1) is the store base for every sh; ret -> $2 (v0) is only
 *     the return value; value0/1/2 -> $9/$10/$11 (t1/t2/t3) cycle the loads.
 *     The barriers allocate all three values without hard register names.
 *   - The first tied barrier keeps `ret = dst` from floating above the first
 *     load and assigns value0 to $t1, so `move v0,a1` lands in that load's
 *     delay slot (offset 0x4), matching retail.
 *   - The second tied barrier assigns the first value1 load to $t2, reserves
 *     $t3 for value2 through an output overwritten before its first C use, and
 *     hides $2==dst from the optimizer. Stores therefore remain addressed
 *     through $5 rather than reusing $2 as the base pointer.
 */
Matrix *TransposeMatrix(Matrix *src, Matrix *dst) {
    s16 *srcp = (s16 *)src;
    s16 *dstp = (s16 *)dst;
    Matrix *ret;
    s32 value0;
    s32 value1;
    s32 value2;

    value0 = srcp[0];
    __asm__ volatile("" : "=r"(value0) : "0"(value0) : "$2", "$3", "$6", "$7", "$8");
    ret = dst;
    dstp[0] = value0;
    value1 = srcp[3];
    __asm__ volatile("" : "=r"(ret), "=r"(value1), "=r"(value2) : "0"(ret), "1"(value1) : "$3", "$6", "$7", "$8", "$9");
    value0 = srcp[6];
    dstp[1] = value1;
    value2 = srcp[1];
    dstp[2] = value0;
    value1 = srcp[4];
    dstp[3] = value2;
    value0 = srcp[7];
    dstp[4] = value1;
    value2 = srcp[2];
    dstp[5] = value0;
    value1 = srcp[5];
    dstp[6] = value2;
    value0 = srcp[8];
    dstp[7] = value1;
    dstp[8] = value0;
    return ret;
}

/*
 * HANDWRITTEN_ASM - excluded from progress; see README.md.
 *
 * Symbol:   func_80069D18 = RotMatrix (PSY-Q libgte; Sony's Run-Time Library
 *           Reference, 8-140). Builds a rotation MATRIX from an SVECTOR of
 *           Euler angles using the packed cos/sin table at g_RCosSinTable (cos in
 *           the high halfword, sin in the low halfword; sin negated for
 *           negative angles).
 * Reason:   hand-written PSY-Q libgte SDK assembly, not compiler C.
 * Evidence: every fixed-point product is a NARROW unsigned multiply keeping
 *           only the low word - 14x `multu`, 15x `mflo`, and ZERO `mfhi` in
 *           the whole body. GCC 2.6.3 and 2.7.2 (this repo's cc1) canonicalise
 *           every truncated 32-bit multiply to signed `mult`, and only emit
 *           `multu` for a true 64-bit widening multiply, which always reads
 *           `mfhi`. `multu` with no `mfhi` is therefore unreachable from C,
 *           and no matched function in this project emits it. The body also
 *           loads the packed table entry as one `lw` and splits it with
 *           `sll 16; sra 16` / `sra 16` where cc1 folds such halfword
 *           extraction into `lh`, and joins its if/else arms with a bare `j`
 *           whose delay slot carries the shared `sra $t1,$t9,16`.
 *           Exact same idiom and register file (t0-t9 only) as the documented
 *           siblings func_80069110 (ScaleMatrixL) and func_80069458.
 * Revisit:  only if the exact cc1 variant that emits narrow multu is obtained
 *           AND verified not to regress already-matched functions.
 */
HANDWRITTEN_ASM("src/main/PAL/lib/libgte", rotation_matrix);
