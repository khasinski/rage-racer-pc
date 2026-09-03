#include <libgte.h>

#include <stdint.h>

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output) {
    MATRIX result = *output;
    int row;
    int column;
    int inner;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            int64_t value = 0;

            for (inner = 0; inner < 3; inner++) {
                value += (int64_t)left->m[row][inner] *
                         right->m[inner][column];
            }
            result.m[row][column] = (short)(value >> 12);
        }
    }
    *output = result;
    return output;
}
