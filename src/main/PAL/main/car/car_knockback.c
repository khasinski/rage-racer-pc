#include "game/diagnostics.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"

#include <stdlib.h>

typedef union CarSpeedAddress {
    s32 *value;
    u16 *magnitude;
} CarSpeedAddress;

void ApplyCarKnockback(GameCarRuntime *car) {
    u32 timer;

    if (car->motionActive != 0) {
        timer = car->motionTimer - 1;
        car->motionTimer = timer;
        if ((s16)timer <= 0) {
            car->motionActive = 0;
            car->motionTimer = 0;
        }

        car->x -= car->velocityX;
        car->z -= car->velocityZ;
        car->velocityX = car->velocityX * 7 / 8;
        car->velocityZ = car->velocityZ * 7 / 8;
    }
}


/*
 * Collision / boundary response: sets the car's knock-back motion vector
 * (velocityX / velocityZ, motionTimer, motionActive) from a push direction and
 * speed derived per `mode` (0 / 2 / 4 select different angle+speed math from
 * x/z). Register-pinned, goto-structured decompilation; do not restructure.
 */


void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode) {
    GameCarRuntime *carReg;
    s32 hitX;
    s32 angle;
    s32 savedAngle;
    s32 hitZ = z;
    s32 adjustedReg;
    s32 raw;
    s32 rawArg;
    s32 adjusted;
    s32 fieldA4;
    s32 tmp;
    s32 speed;
    s32 trig;
    s32 product;
    u32 hitSign;
    static int traceEnabled = -1;
    static int traceTimer = -1;
    static int traceTimerMin = -1;
    static int traceTimerMax = -1;

    carReg = car;
    
    hitX = x;
    carReg->motionActive = 1;
    if (mode < 2) {

    adjustedReg = carReg->trackHeading.half.low;
    raw = 0xC00;
    raw -= adjustedReg;
    rawArg = raw;
    raw = (s32)((u32)raw << 16);
    adjustedReg = carReg->trackLateralOffset;
    raw >>= 16;
    if (adjustedReg < 0) {
        adjusted = raw - 0x400;
    } else {
        adjusted = raw + 0x400;
    }
    adjustedReg = adjusted & 0xFFF;
    
    fieldA4 = carReg->speed;
    angle = adjustedReg;
    if (fieldA4 >= 0x321) {
        if (fieldA4 >= 0x709) {
            speed = 0x708;
        } else {
            CarSpeedAddress speedAddress;

            speedAddress.value = &carReg->speed;
            speed = *speedAddress.magnitude;
        }
        trig = rsin(GetAngleDistance((s16)rawArg, carReg->bodyYaw));
        product = (s16)speed * trig;
        tmp = product / 65536;
    } else {
        trig = rsin(GetAngleDistance((s16)rawArg, carReg->bodyYaw));
        product = trig * 2;
        product += trig;
        product <<= 3;
        product += trig;
        adjustedReg = product * 2;
        tmp = adjustedReg / 4096;
    }

    speed = tmp + 10;

    savedAngle = angle;
    trig = rsin(savedAngle);
    product = speed << 16;
    angle = product >> 16;
    adjustedReg = trig * angle;
    hitX = adjustedReg / 4096;
    trig = rcos(savedAngle);
    adjustedReg = trig * angle;
    hitZ = adjustedReg / 4096;
    tmp = 0x1E;
    } else if (mode < 4) {
    adjustedReg = carReg->trackHeading.half.low;
    raw = 0xC00;
    raw -= adjustedReg;
    raw <<= 16;
    adjustedReg = carReg->trackLateralOffset;
    raw >>= 16;
    if (adjustedReg < 0) {
        adjusted = raw - 0x400;
    } else {
        adjusted = raw + 0x400;
    }
    rawArg = adjusted & 0xFFF;
    angle = rawArg;
    trig = rsin(rawArg);
    tmp = trig * 20;
    hitX = tmp / 4096;
    trig = rcos(angle);
    tmp = trig * 20;
    hitZ = tmp / 4096;
    tmp = 0xF;
    } else {
    hitSign = hitX;
    adjustedReg = hitSign >> 31;
    adjustedReg = hitX + adjustedReg;
    hitX = adjustedReg >> 1;
    hitSign = hitZ;
    adjustedReg = hitSign >> 31;
    adjustedReg = hitZ + adjustedReg;
    hitZ = adjustedReg >> 1;
    tmp = 0xF;
    }

    carReg->motionTimer = tmp;
    carReg->velocityX = hitX;
    carReg->velocityZ = hitZ;
    if (traceEnabled < 0) {
        const char *timerText = DiagnosticsValue("car.knockback_trace_timer");
        const char *timerMinText = DiagnosticsValue("car.knockback_trace_timer_min");
        const char *timerMaxText = DiagnosticsValue("car.knockback_trace_timer_max");
        traceEnabled = DiagnosticsEnabled("car.knockback_trace");
        traceTimer = timerText != NULL ? (int)strtol(timerText, NULL, 0) : -1;
        traceTimerMin = timerMinText != NULL ? (int)strtol(timerMinText, NULL, 0) : -1;
        traceTimerMax = timerMaxText != NULL ? (int)strtol(timerMaxText, NULL, 0) : -1;
    }
    if (traceEnabled &&
        (traceTimer < 0 || g_SceneTimer == traceTimer) &&
        (traceTimerMin < 0 || g_SceneTimer >= traceTimerMin) &&
        (traceTimerMax < 0 || g_SceneTimer <= traceTimerMax)) {
        Trace("car-knockback", "timer=%d player=%d input=%d,%d mode=%d "
               "output=%d,%d duration=%d heading=%d lateral=%d", g_SceneTimer,
               carReg == (GameCarRuntime *)&g_PlayerCar, x, z, mode,
               carReg->velocityX, carReg->velocityZ, carReg->motionTimer,
               carReg->trackHeading.half.low, carReg->trackLateralOffset);
    }
}

