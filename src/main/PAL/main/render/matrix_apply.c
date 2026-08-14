#include "common.h"
#include "psyq/gte.h"
#include "game/render.h"

typedef union MatrixElementAddress {
    s16 *elements;
    s32 *packed;
} MatrixElementAddress;

/*
 * Applies the 3x3 fixed-point matrix `mtx` to the vector (x,y,z), writing each
 * component to the three output pointers (result >> 12). Each row has an identity
 * fast-path: if the row is (1.0, 0, 0) etc. the matching input component is
 * copied straight through without the multiply-accumulate.
 */
void MatrixApplyVectorComponents(Matrix *mtx, s32 x, s32 y, s32 z, s32 *outX, s32 *outY, s32 *outZ) {
    s16 *m = mtx->m[0];
    MatrixElementAddress elementAddress;

    elementAddress.elements = &m[0];
    if (*elementAddress.packed == 0x1000 && m[2] == 0) {
        *outX = x;
    } else {
        s32 result = m[0] * x;

        result += m[1] * y;
        result += m[2] * z;
        *outX = result >> 12;
    }

    elementAddress.elements = &m[4];
    if (m[3] == 0 && *elementAddress.packed == 0x1000) {
        *outY = y;
    } else {
        s32 result = m[3] * x;

        result += m[4] * y;
        result += m[5] * z;
        *outY = result >> 12;
    }

    elementAddress.elements = &m[6];
    if (*elementAddress.packed == 0 && m[8] == 0x1000) {
        *outZ = z;
    } else {
        s32 result = m[6] * x;

        result += m[7] * y;
        result += m[8] * z;
        *outZ = result >> 12;
    }
}

/* HANDWRITTEN_ASM - PSY-Q libgte hand-asm (matrix/GTE), excluded from progress. */

#define FAST_LOAD(offset)                              \
    do {                                               \
        s32 value;                                     \
        __asm__ volatile("lw %0, %1(%2)"               \
                         : "=r"(value)                 \
                         : "i"(offset), "r"(v));       \
        out[(offset) / 4] = value;                     \
    } while (0)

#define DOT_ROW(row, dst)                              \
    do {                                               \
        register s32 value asm("$2");                  \
        __asm__ volatile(                              \
            "lh    $3, %1(%5)\n\t"                     \
            "lw    %0, 0(%4)\n\t"                      \
            "nop\n\t"                                  \
            "mult  $3, %0\n\t"                         \
            "lh    $4, %2(%5)\n\t"                     \
            "mflo  %0\n\t"                             \
            "lw    $3, 4(%4)\n\t"                      \
            "nop\n\t"                                  \
            "mult  $4, $3\n\t"                         \
            "lh    $4, %3(%5)\n\t"                     \
            "mflo  $5\n\t"                             \
            "lw    $3, 8(%4)\n\t"                      \
            "nop\n\t"                                  \
            "mult  $4, $3\n\t"                         \
            "addu  %0, %0, $5\n\t"                     \
            "mflo  $3\n\t"                             \
            "addu  %0, %0, $3\n\t"                     \
            "sra   %0, %0, 12"                         \
            : "=r"(value)                              \
            : "i"((row) + 0), "i"((row) + 2), "i"((row) + 4), "r"(v), "r"(m) \
            : "$3", "$4", "$5", "hi", "lo");         \
        out[(dst)] = value;                            \
    } while (0)

void MatrixApplyVector(s16 *mtx, s32 *vec, s32 *out) {
    s16 *m = mtx;
    s32 *v = vec;
    MatrixElementAddress elementAddress;

    elementAddress.elements = &m[0];
    if (*elementAddress.packed == 0x1000 && m[2] == 0) {
        FAST_LOAD(0);
    } else {
        DOT_ROW(0, 0);
    }

    elementAddress.elements = &m[4];
    if (m[3] == 0 && *elementAddress.packed == 0x1000) {
        FAST_LOAD(4);
    } else {
        DOT_ROW(6, 1);
    }

    elementAddress.elements = &m[6];
    if (*elementAddress.packed == 0 && m[8] == 0x1000) {
        FAST_LOAD(8);
    } else {
        DOT_ROW(12, 2);
    }
}

void MatrixApplyZRotation(Matrix *mtx, s32 degrees) {
    Matrix sp10;
    s32 angle;
    s32 c;
    s32 s;
    s16 s_copy;

    angle = degrees / 360;
    c = rcos(angle);
    s = rsin(angle);
    s_copy = s;

    if (degrees != 0) {
        sp10.m[0][0] = c;
        sp10.m[0][1] = -s;
        sp10.m[0][2] = 0;
        sp10.m[1][0] = s_copy;
        sp10.m[1][1] = c;
        sp10.m[1][2] = 0;
        sp10.m[2][0] = 0;
        sp10.m[2][1] = 0;
        sp10.m[2][2] = 0x1000;
        sp10.t[0] = 0;
        sp10.t[1] = 0;
        sp10.t[2] = 0;
        MulMatrix(mtx, &sp10);
    }
}
