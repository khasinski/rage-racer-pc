/*
 * Two of the console's matrix ops the runtime does not carry.
 *
 * These are the real thing rather than stand-ins, which is why they live apart
 * from the port's stubs. They take libgte's own types, so this file stays
 * clear of the header that widens those signatures for decompiled callers.
 */

#include <libgte.h>
#include <limits.h>
#include <stdint.h>

static short GteRegisterValue(int64_t value) {
    uint16_t bits = (uint16_t)value;

    if (bits <= INT16_MAX) return (short)bits;
    return (short)((int32_t)bits - INT32_C(0x10000));
}

MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    MATRIX result = *right;
    int row;
    int column;
    int inner;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            int64_t sum = 0;
            for (inner = 0; inner < 3; inner++) {
                sum += (int64_t)left->m[row][inner] *
                       right->m[inner][column];
            }
            result.m[row][column] = GteRegisterValue(sum >> 12);
        }
    }
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            right->m[row][column] = result.m[row][column];
        }
    }
    return right;
}

short *ApplyMatrixSV(void *matrix, void *input, short *output) {
    MATRIX *m = matrix;
    SVECTOR *v = input;
    int row;

    for (row = 0; row < 3; row++) {
        int64_t sum = (int64_t)m->m[row][0] * v->vx
                    + (int64_t)m->m[row][1] * v->vy
                    + (int64_t)m->m[row][2] * v->vz;
        output[row] = GteRegisterValue(sum >> 12);
    }
    return output;
}
