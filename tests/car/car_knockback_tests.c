#include "common.h"
#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/player_car_internal.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

PlayerCarRuntime g_PlayerCar;
s32 g_SceneTimer;

int DiagnosticsEnabled(const char *key) {
    (void)key;
    return 0;
}

const char *DiagnosticsValue(const char *key) {
    (void)key;
    return NULL;
}

int DiagnosticsIntValue(const char *key, int fallback) {
    (void)key;
    return fallback;
}

void Trace(const char *topic, const char *format, ...) {
    (void)topic;
    (void)format;
}

static unsigned long s_digest = 2166136261UL;

static void Fold(s32 value) {
    u32 bits = (u32)value;
    int byte;

    for (byte = 0; byte < 4; byte++) {
        s_digest ^= (bits >> (byte * 8)) & 0xFF;
        s_digest = (s_digest * 16777619UL) & 0xFFFFFFFFUL;
    }
}

static void SweepTrackBoundaryKnockback(void) {
    static const s16 headings[] = {0, 0x400, 0xC00, -1};
    static const s32 laterals[] = {-100, 0, 100};
    static const s32 speeds[] = {0, 0x320, 0x321, 0x708, 0x709, 0x10001};
    static const s16 yaws[] = {0, 0x400, 0x900};
    static const s32 inputs[][2] = {{100, 51}, {-101, -53}};
    size_t hi, li, si, yi, ii;
    s32 mode;

    for (hi = 0; hi < sizeof(headings) / sizeof(headings[0]); hi++)
    for (li = 0; li < sizeof(laterals) / sizeof(laterals[0]); li++)
    for (si = 0; si < sizeof(speeds) / sizeof(speeds[0]); si++)
    for (yi = 0; yi < sizeof(yaws) / sizeof(yaws[0]); yi++)
    for (ii = 0; ii < sizeof(inputs) / sizeof(inputs[0]); ii++)
    for (mode = 0; mode <= 5; mode++) {
        GameCarRuntime car;

        memset(&car, 0, sizeof(car));
        car.trackHeading.half.low = headings[hi];
        car.trackLateralOffset = laterals[li];
        car.speed = speeds[si];
        car.bodyYaw = yaws[yi];
        SetTrackBoundaryKnockback(
            &car, inputs[ii][0], inputs[ii][1], (CarTrackContact)mode);
        Fold(car.motionActive);
        Fold(car.motionTimer);
        Fold(car.velocityX);
        Fold(car.velocityZ);
    }
}

static void SweepApplyCarKnockback(void) {
    static const u16 timers[] = {0, 1, 2, 0x7FFF, 0x8000};
    static const s16 velocities[] = {-101, -1, 0, 1, 100};
    size_t ti, xi, zi;
    s32 active;

    for (active = 0; active <= 1; active++)
    for (ti = 0; ti < sizeof(timers) / sizeof(timers[0]); ti++)
    for (xi = 0; xi < sizeof(velocities) / sizeof(velocities[0]); xi++)
    for (zi = 0; zi < sizeof(velocities) / sizeof(velocities[0]); zi++) {
        GameCarRuntime car;

        memset(&car, 0, sizeof(car));
        car.motionActive = (s16)active;
        car.motionTimer = timers[ti];
        car.x = 12345;
        car.z = -23456;
        car.velocityX = velocities[xi];
        car.velocityZ = velocities[zi];
        ApplyCarKnockback(&car);
        Fold(car.motionActive);
        Fold(car.motionTimer);
        Fold(car.x);
        Fold(car.z);
        Fold(car.velocityX);
        Fold(car.velocityZ);
    }
}

int main(void) {
    static const unsigned long expected = 1747374351UL;
    GameCarRuntime car;

    SweepTrackBoundaryKnockback();
    SweepApplyCarKnockback();
    if (s_digest != expected) {
        printf("FAIL car knockback digest %lu, expected %lu\n",
               s_digest, expected);
        return 1;
    }

    memset(&car, 0, sizeof(car));
    car.motionActive = 1;
    car.motionTimer = 2;
    car.x = INT_MIN;
    car.z = INT_MAX;
    car.velocityX = 1;
    car.velocityZ = -1;
    ApplyCarKnockback(&car);
    if (car.x != INT_MAX || car.z != INT_MIN) {
        puts("FAIL knockback position did not wrap like the PS1");
        return 1;
    }

    memset(&car, 0, sizeof(car));
    SetCarCollisionKnockback(&car, INT_MAX, INT_MIN);
    if (car.velocityX != -1 || car.velocityZ != 0) {
        puts("FAIL collision knockback vector did not keep its low halfword");
        return 1;
    }
    puts("car knockback preserves its collision modes and decay");
    return 0;
}
