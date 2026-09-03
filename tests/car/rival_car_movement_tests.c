#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/render.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
GameRenderState g_RenderState;

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output) {
    (void)left;
    (void)right;
    return output;
}

void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z, int32_t pitch,
                              int32_t yaw, int32_t roll) {
    (void)x;
    (void)y;
    (void)z;
    (void)pitch;
    (void)yaw;
    (void)roll;
}

static u32 s_digest = 2166136261U;

static void Fold(s32 value) {
    int byte;

    for (byte = 0; byte < 4; byte++) {
        s_digest ^= ((u32)value >> (byte * 8)) & 0xFF;
        s_digest *= 16777619U;
    }
}

int main(void) {
    static const s32 slots[] = {0, 3, 4, 10};
    static const s32 headings[] = {0, 0x400, 0x900};
    static const s32 speeds[] = {0, 801, 1600};
    static const s32 steering[] = {-301, -300, -65, -64, 64, 65, 300, 301};
    static const s32 yawRates[] = {-20, 0, 20};
    static const u32 expected = 3905550109U;
    size_t si, hi, vi, ti, yi;
    int cases = 0;

    for (si = 0; si < sizeof(slots) / sizeof(slots[0]); si++)
    for (hi = 0; hi < sizeof(headings) / sizeof(headings[0]); hi++)
    for (vi = 0; vi < sizeof(speeds) / sizeof(speeds[0]); vi++)
    for (ti = 0; ti < sizeof(steering) / sizeof(steering[0]); ti++)
    for (yi = 0; yi < sizeof(yawRates) / sizeof(yawRates[0]); yi++) {
        GameCarRuntime *car;
        s32 index;

        memset(g_Cars, 0, sizeof(g_Cars));
        for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
            g_Cars[index].activeFlag = -1;
        }
        car = &g_Cars[slots[si]];
        car->activeFlag = 0;
        car->x = 10000;
        car->z = -20000;
        car->motionX = 123;
        car->motionY = -77;
        car->motionZ = -234;
        car->field_1C = 0x12345678;
        car->bodyPitch = 0x123;
        car->bodyYaw = -0x234;
        car->bodyRoll = 0x345;
        car->headingAngle = headings[hi];
        car->speed = speeds[vi];
        car->steeringAngle = steering[ti];
        car->bodyRollVelocity = -9;
        car->worldVelocityX = 320;
        car->worldVelocityZ = -640;
        car->yawRate = yawRates[yi];

        MoveRivalCars();
        if (car->field_1C != 0x12345678) {
            puts("rival lean overwrote state following its motion vector");
            return 1;
        }
        Fold(car->x);
        Fold(car->z);
        Fold(car->motionX);
        Fold(car->motionY);
        Fold(car->motionZ);
        Fold(car->worldVelocityX);
        Fold(car->worldVelocityZ);
        Fold(car->baseBodyYaw);
        Fold(car->bodyYaw);
        Fold(car->steeringAngle);
        Fold(car->bodyRollVelocity);
        cases++;
    }

    if (s_digest != expected) {
        printf("rival movement: %d cases folded to %u, expected %u\n",
               cases, s_digest, expected);
        return 1;
    }

    memset(g_Cars, 0, sizeof(g_Cars));
    for (si = 0; si < RACE_CAR_SLOT_COUNT; si++) {
        g_Cars[si].activeFlag = -1;
    }
    g_Cars[4].activeFlag = 0;
    g_Cars[4].speed = INT_MIN;
    g_Cars[4].yawRate = INT_MIN;
    g_Cars[4].bodyRollVelocity = INT_MAX;
    MoveRivalCars();
    if (g_Cars[4].worldVelocityX != 0 ||
        g_Cars[4].worldVelocityZ != 0 ||
        g_Cars[4].steeringAngle != -300 ||
        g_Cars[4].bodyYaw != INT_MIN ||
        g_Cars[4].bodyRollVelocity != 268435455) {
        puts("rival movement did not preserve word wrapping at extremes");
        return 1;
    }

    g_Cars[4].activeFlag = -1;
    g_Cars[0].activeFlag = 0;
    g_Cars[0].yawRate = INT_MIN;
    MoveRivalCars();
    if (g_Cars[0].steeringAngle != -300 ||
        g_Cars[0].bodyYaw != INT_MIN) {
        puts("detailed rival lean did not handle the minimum yaw rate");
        return 1;
    }
    printf("rival movement preserves %d fixed-point states\n", cases);
    return 0;
}
