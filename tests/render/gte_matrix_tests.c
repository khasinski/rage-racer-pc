#include <libgte.h>
#include "psyq/gte.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #expression); \
        return 1; \
    } \
} while (0)

static int TestMatrixMultiply(void) {
    MATRIX left;
    MATRIX right;

    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    left.m[0][0] = 4096;
    left.m[1][1] = 4096;
    left.m[2][2] = 4096;
    right.m[0][0] = 100;
    right.m[1][1] = 200;
    right.m[2][2] = 300;
    right.t[0] = 11;
    right.t[1] = 22;
    right.t[2] = 33;

    CHECK(MulMatrix2(&left, &right) == &right);
    CHECK(right.m[0][0] == 100 && right.m[1][1] == 200 &&
          right.m[2][2] == 300);
    CHECK(right.t[0] == 11 && right.t[1] == 22 && right.t[2] == 33);
    return 0;
}

static int TestWideAccumulator(void) {
    MATRIX matrix;
    MATRIX right;
    SVECTOR vector = {32767, 32767, 32767, 0};
    short output[3];
    const short expected = -48;
    int row;
    int column;

    memset(&matrix, 0, sizeof(matrix));
    memset(&right, 0, sizeof(right));
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            matrix.m[row][column] = 32767;
            right.m[row][column] = 32767;
        }
    }

    CHECK(ApplyMatrixSV(&matrix, &vector, output) == output);
    CHECK(output[0] == expected && output[1] == expected &&
          output[2] == expected);
    MulMatrix2(&matrix, &right);
    CHECK(right.m[0][0] == expected && right.m[1][1] == expected &&
          right.m[2][2] == expected);
    return 0;
}

int main(void) {
    if (TestMatrixMultiply() != 0) return 1;
    if (TestWideAccumulator() != 0) return 1;
    puts("GTE matrix helpers retain translation and use wide accumulators");
    return 0;
}
