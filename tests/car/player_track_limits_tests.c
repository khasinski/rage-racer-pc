#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CarHullPoint g_CarCornerOffsets[4];
s32 g_SceneTimer;

static int s_traceEnabled;
static const char *s_traceTimer;
static int s_enabledCalls;
static int s_valueCalls;
static int s_traceCalls;
static int s_failures;

int DiagnosticsEnabled(const char *key) {
    (void)key;
    s_enabledCalls++;
    return s_traceEnabled;
}

const char *DiagnosticsValue(const char *key) {
    (void)key;
    s_valueCalls++;
    return s_traceTimer;
}

int DiagnosticsIntValue(const char *key, int fallback) {
    const char *text = DiagnosticsValue(key);

    return text != NULL ? (int)strtol(text, NULL, 0) : fallback;
}

void Trace(const char *channel, const char *format, ...) {
    (void)channel;
    (void)format;
    s_traceCalls++;
}

static void CheckLimits(const Matrix *matrix, s32 right, s32 left,
                        s32 rightMode, s32 leftMode) {
    CarTrackLimits limits;

    memset(&limits, 0x7F, sizeof(limits));
    MeasurePlayerTrackLimits(matrix, &limits);
    if (limits.rightInset != right || limits.leftInset != left ||
        limits.rightKnockbackMode != rightMode ||
        limits.leftKnockbackMode != leftMode) {
        printf("FAIL limits: right=%d/%d left=%d/%d modes=%d/%d,%d/%d\n",
               limits.rightInset, right, limits.leftInset, left,
               limits.rightKnockbackMode, rightMode,
               limits.leftKnockbackMode, leftMode);
        s_failures++;
    }
}

int main(void) {
    Matrix matrix;

    memset(&matrix, 0, sizeof(matrix));
    g_CarCornerOffsets[0].x = -10;
    g_CarCornerOffsets[0].z = 5;
    g_CarCornerOffsets[1].x = 20;
    g_CarCornerOffsets[1].z = -30;
    g_CarCornerOffsets[2].x = -30;
    g_CarCornerOffsets[2].z = 40;
    g_CarCornerOffsets[3].x = 15;
    g_CarCornerOffsets[3].z = -50;

    matrix.m[0][0] = 4096;
    CheckLimits(&matrix, 80, -120, 2, 3);

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[0][2] = 4096;
    CheckLimits(&matrix, 160, -200, 3, 4);

    memset(&matrix, 0, sizeof(matrix));
    s_traceEnabled = 1;
    s_traceTimer = "123";
    g_SceneTimer = 123;
    s_enabledCalls = 0;
    s_valueCalls = 0;
    s_traceCalls = 0;
    CheckLimits(&matrix, 0, -1, 1, 0);
    if (s_enabledCalls != 1 || s_valueCalls != 1 || s_traceCalls != 4) {
        printf("FAIL trace calls: enabled=%d value=%d trace=%d\n",
               s_enabledCalls, s_valueCalls, s_traceCalls);
        s_failures++;
    }

    memset(&matrix, 0, sizeof(matrix));
    matrix.m[0][0] = 4096;
    g_CarCornerOffsets[0].x = INT16_MAX;
    g_CarCornerOffsets[1].x = INT16_MIN;
    g_CarCornerOffsets[2].x = 1;
    g_CarCornerOffsets[3].x = -1;
    g_CarCornerOffsets[0].z = 0;
    g_CarCornerOffsets[1].z = 0;
    g_CarCornerOffsets[2].z = 0;
    g_CarCornerOffsets[3].z = 0;
    s_traceEnabled = 0;
    CheckLimits(&matrix, 4, -4, 3, 1);

    if (s_failures != 0) {
        printf("%d player track limit checks failed\n", s_failures);
        return 1;
    }
    puts("player track limits select the outermost corners");
    return 0;
}
