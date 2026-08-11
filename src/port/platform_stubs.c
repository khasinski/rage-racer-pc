#include "rage/compat.h"
#include <libgte.h>

long SpuTransferStatus(void *address, long mode) {
    (void)address;
    (void)mode;
    return 0;
}

MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    MATRIX result = *right;
    int row;
    int column;
    int inner;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            long sum = 0;
            for (inner = 0; inner < 3; inner++) {
                sum += (long)left->m[row][inner] * right->m[inner][column];
            }
            result.m[row][column] = (short)(sum >> 12);
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
        long sum = (long)m->m[row][0] * v->vx
                 + (long)m->m[row][1] * v->vy
                 + (long)m->m[row][2] * v->vz;
        output[row] = (short)(sum >> 12);
    }
    return output;
}
