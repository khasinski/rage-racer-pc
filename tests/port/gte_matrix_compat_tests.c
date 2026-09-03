#include <libgte.h>

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void SetDiagonal(MATRIX *matrix, short x, short y, short z) {
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = x;
    matrix->m[1][1] = y;
    matrix->m[2][2] = z;
}

static int TestSeparateOutput(void) {
    MATRIX left;
    MATRIX right;
    MATRIX output;

    SetDiagonal(&left, 4096, 2048, -4096);
    SetDiagonal(&right, 100, 200, 300);
    memset(&output, 0, sizeof(output));
    output.t[0] = 11;
    output.t[1] = 22;
    output.t[2] = 33;
    CHECK(MulMatrix0(&left, &right, &output) == &output);
    CHECK(output.m[0][0] == 100 && output.m[1][1] == 100 &&
          output.m[2][2] == -300);
    CHECK(output.t[0] == 11 && output.t[1] == 22 && output.t[2] == 33);
    return 0;
}

static int TestAliasedOutput(void) {
    MATRIX left;
    MATRIX right;

    SetDiagonal(&left, 4096, 2048, -4096);
    SetDiagonal(&right, 100, 200, 300);
    right.t[0] = 41;
    CHECK(MulMatrix0(&left, &right, &right) == &right);
    CHECK(right.m[0][0] == 100 && right.m[1][1] == 100 &&
          right.m[2][2] == -300 && right.t[0] == 41);

    SetDiagonal(&left, 4096, 2048, -4096);
    SetDiagonal(&right, 100, 200, 300);
    left.t[1] = 52;
    CHECK(MulMatrix0(&left, &right, &left) == &left);
    CHECK(left.m[0][0] == 100 && left.m[1][1] == 100 &&
          left.m[2][2] == -300 && left.t[1] == 52);
    return 0;
}

static int TestWideAccumulator(void) {
    MATRIX left;
    MATRIX right;
    MATRIX output;
    int row;
    int column;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            left.m[row][column] = 32767;
            right.m[row][column] = 32767;
        }
    }
    memset(&output, 0, sizeof(output));
    MulMatrix0(&left, &right, &output);
    CHECK(output.m[0][0] == -48 && output.m[2][2] == -48);
    return 0;
}

int main(void) {
    CHECK(TestSeparateOutput() == 0);
    CHECK(TestAliasedOutput() == 0);
    CHECK(TestWideAccumulator() == 0);
    puts("MulMatrix0 preserves translation, aliasing and wide products");
    return 0;
}
