#include "game/car.h"
#include "game/car_track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static CarHullPoint corners[4];

static int s_failures;

static void CheckLimits(const Matrix *matrix, s32 right, s32 left,
                        s32 rightMode, s32 leftMode) {
    CarTrackLimits limits;

    memset(&limits, 0x7F, sizeof(limits));
    MeasureCarTrackLimits(matrix, corners, &limits);
    if (limits.rightInset != right || limits.leftInset != left ||
        limits.rightContact != rightMode ||
        limits.leftContact != leftMode) {
        printf("FAIL limits: right=%d/%d left=%d/%d modes=%d/%d,%d/%d\n",
               limits.rightInset, right, limits.leftInset, left,
               limits.rightContact, rightMode,
               limits.leftContact, leftMode);
        s_failures++;
    }
}

int main(void) {
    Matrix matrix;

    memset(&matrix, 0, sizeof(matrix));
    corners[0].x = -10;
    corners[0].z = 5;
    corners[1].x = 20;
    corners[1].z = -30;
    corners[2].x = -30;
    corners[2].z = 40;
    corners[3].x = 15;
    corners[3].z = -50;

    matrix.m[0][0] = 4096;
    CheckLimits(&matrix, 80, -120, 2, 3);

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[0][2] = 4096;
    CheckLimits(&matrix, 160, -200, 3, 4);

    memset(&matrix, 0, sizeof(matrix));
    CheckLimits(&matrix, 0, -1, 1, 0);

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[0][0] = 4096;
    corners[0].x = INT16_MAX;
    corners[1].x = INT16_MIN;
    corners[2].x = 1;
    corners[3].x = -1;
    corners[0].z = 0;
    corners[1].z = 0;
    corners[2].z = 0;
    corners[3].z = 0;
    CheckLimits(&matrix, 4, -4, 3, 1);

    if (s_failures != 0) {
        printf("%d player track limit checks failed\n", s_failures);
        return 1;
    }
    puts("player track limits select the outermost corners");
    return 0;
}
