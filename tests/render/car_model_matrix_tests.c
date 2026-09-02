#include "game/car_model_matrix.h"

#include <stdio.h>
#include <string.h>

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            fprintf(stderr, "%s:%d: got %d, expected %d\n", __FILE__,        \
                    __LINE__, (s32)(actual), (s32)(expected));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    Matrix source;
    Matrix flipped;

    memset(&source, 0, sizeof(source));
    source.m[0][0] = 1;
    source.m[0][1] = 2;
    source.m[0][2] = 3;
    source.m[1][0] = 4;
    source.m[1][1] = 5;
    source.m[1][2] = 6;
    source.m[2][0] = 7;
    source.m[2][1] = 8;
    source.m[2][2] = 9;
    source.t[0] = 10;
    source.t[1] = 11;
    source.t[2] = 12;

    FlipMatrixXZColumns(&flipped, &source);
    CHECK_EQ(flipped.m[0][0], -1);
    CHECK_EQ(flipped.m[0][1], 2);
    CHECK_EQ(flipped.m[0][2], -3);
    CHECK_EQ(flipped.m[2][0], -7);
    CHECK_EQ(flipped.m[2][1], 8);
    CHECK_EQ(flipped.m[2][2], -9);
    CHECK_EQ(flipped.t[0], 10);
    CHECK_EQ(flipped.t[2], 12);

    FlipMatrixXZColumns(&source, &source);
    CHECK_EQ(source.m[1][0], -4);
    CHECK_EQ(source.m[1][1], 5);
    CHECK_EQ(source.m[1][2], -6);
    CHECK_EQ(source.t[1], 11);

    puts("car model matrix tests passed");
    return 0;
}
