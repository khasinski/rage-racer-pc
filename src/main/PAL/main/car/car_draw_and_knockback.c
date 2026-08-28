#include "common.h"
#include "game/diagnostics.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"
#include "game/random.h"

#include <stdio.h>
#include <stdlib.h>

typedef union CarSpeedAddress {
    s32 *value;
    u16 *magnitude;
} CarSpeedAddress;


/*
 * Updates the car's skid/tilt counter, clamping it
 * against the spec block's redline value (g_CarSpec + 0x106). Register-pinned,
 * goto-structured; the raw drive-block reads (+0xA2 / +0x34) preserve the match.
 */

/*
 * Applies and decays the collision shake: while motionActive/motionTimer are
 * set, subtracts field_7C / field_7E from the car position and damps the
 * velocity by 7/8 (*8-1 >> 3) each frame. field_7C / field_7E are read via
 * inline __asm__ lh loads and the registers are pinned, so the struct offsets
 * must not change.
 */

void BuildStartingGrid(void) {
    GameCarRuntime *entryBase;
    register RaceGridSlot *table asm("s3");
    s32 i;
    register s16 *flagPtr asm("s1");
    s32 one;
    RaceGridSlot *cursor;
    s32 state;
    u16 track;

    entryBase = g_Cars;
    state = g_SceneId;
    g_ClosestRivalRank = 3;

    if (state == 0xB) {
        table = g_RaceGridSlots;
        i = 0;
        one = 1;
        flagPtr = &entryBase->activeFlag;
        g_RaceSeries = g_GrandPrixSeries;
        cursor = table;
        do {
            track = ReadRaceTrackDirection();
            *flagPtr = 0;
            flagPtr[6] = track;
            if (cursor->value >= 0) {
                ClearCarMotionState(entryBase);
                *flagPtr = one;
                InitRivalCar(entryBase, i, table);
                InitRivalCarAi(entryBase, i, table);
            } else {
                *flagPtr = 0;
            }
            cursor++;
            i++;
            flagPtr += sizeof(GameCarRuntime) / sizeof(*flagPtr);
            entryBase++;
        } while (i < 0xB);
    } else {
        table = g_AttractGridSlots;
        i = 0;
        one = 1;
        flagPtr = &entryBase->activeFlag;
        g_RaceSeries = g_GrandPrixSeries;
        cursor = table;
        do {
            track = ReadRaceTrackDirection();
            *flagPtr = 0;
            flagPtr[6] = track;
            if (cursor->value >= 0) {
                ClearCarMotionState(entryBase);
                *flagPtr = one;
                InitRivalCar(entryBase, i, table);
                InitRivalCarAi(entryBase, i, table);
            } else {
                *flagPtr = 0;
            }
            cursor++;
            i++;
            flagPtr += sizeof(GameCarRuntime) / sizeof(*flagPtr);
            entryBase++;
        } while (i < 0xB);
    }

    SeedCarRouteMarkers();
}

void DrawCars(void) {
    GameCarRuntime *base;
    s32 i;
    s32 one;
    s32 minus_one;

    base = g_Cars;
    SelectModelBank(1);

    i = 0;
    minus_one = -1;
    one = 1;
    do {
        if (base->activeFlag != (i++, minus_one)) {
            if (base->aiEnabled == one) {
                DrawCar(GetCarRenderObject(base));
            }
        }
        base++;
    } while (i < 11);
}

void DrawPlayerCarOnly(void) {
    SelectModelBank(1);
    DrawCar(GetCarRenderObject(g_Cars));
}

void ClearCarMotionState(GameCarRuntime *car) {
    car->collisionFlag = 0;
    car->motionMode = 0;
    car->motionModeTimer = 0;
    car->motionValue.value = 0;
    car->motionActive = 0;
    car->motionTimer = 0;
    car->velocityX = 0;
    car->velocityZ = 0;
    car->tiltCounter = 0;
    car->reserved8E = 0;
    car->verticalPitch = 0;
    car->bodyKickOffset = 0;
    car->verticalRoll = 0;
    car->reserved96 = 0;
    car->verticalMotionState = 0;
    car->verticalMotionTimer = 0;
    car->verticalMotionRate = 0;
    car->verticalTargetY = 0;
}

void UpdateCarTiltCounter(GameCarRuntime *car) {
    GameCarRuntime *obj;
    GameCarAiBlock *state;
    s32 value;
    register s32 limit asm("$3");

    obj = car;

    state = GetCarAiBlock(obj);
    if (g_RacePhase < 2) {
        value = 8;
    } else {
    if (obj->verticalMotionState == 0) {
        if (obj->engineRpm >= g_CarSpec->redline &&
            obj->acceleratorInput >= 0x81 &&
            obj->slideInput.halves.low == 0) {
            register s32 tilt asm("$4");

            tilt = (u16)obj->tiltCounter;
            value = obj->currentGear;
            tilt -= 4;
            obj->tiltCounter = tilt;
            tilt = (s16)tilt;
            limit = 9 - value;
            value = (limit << 2) + limit;
            value = -value;
            if (tilt < value) {
                obj->tiltCounter = value;
            }
            return;
        }

        if (state->brakeInput >= 0x81 ||
            state->slideInput.halves.low > 0) {
            if (obj->speed >= 0x51) {
                value = (u16)obj->tiltCounter + 2;
                obj->tiltCounter = value;
                value = (s16)value;
                if (value >= 9) {
                    obj->tiltCounter = 8;
                }
                return;
            }
        }
    }

    limit = obj->tiltCounter;
    value = limit * 3;
    if (value < 0) {
        value += 3;
    }
    value >>= 2;
    }
    obj->tiltCounter = value;
}

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
    register s32 savedAngle asm("$19");
    s32 hitZ = z;
    register s32 adjustedReg asm("$2");
    s32 raw;
    register s32 rawArg asm("$4");
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
    asm("" : : "r"(carReg));
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
    asm("" : "=r"(adjustedReg) : "0"(adjustedReg));
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
        if (product < 0) {
            product += 0xFFFF;
        }
        tmp = product >> 16;
    } else {
        trig = rsin(GetAngleDistance((s16)rawArg, carReg->bodyYaw));
        product = trig * 2;
        product += trig;
        product <<= 3;
        product += trig;
        adjustedReg = product * 2;
        if (adjustedReg < 0) {
            adjustedReg += 0xFFF;
        }
        tmp = adjustedReg >> 12;
    }

    speed = tmp + 10;

    savedAngle = angle;
    trig = rsin(savedAngle);
    product = speed << 16;
    angle = product >> 16;
    adjustedReg = trig * angle;
    if (adjustedReg < 0) {
        adjustedReg += 0xFFF;
    }
    hitX = adjustedReg >> 12;
    trig = rcos(savedAngle);
    adjustedReg = trig * angle;
    if (adjustedReg < 0) {
        adjustedReg += 0xFFF;
    }
    hitZ = adjustedReg >> 12;
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
    if (tmp < 0) {
        tmp += 0xFFF;
    }
    hitX = tmp >> 12;
    trig = rcos(angle);
    tmp = trig * 20;
    if (tmp < 0) {
        tmp += 0xFFF;
    }
    hitZ = tmp >> 12;
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

void StartCarBodyKick(s32 strength, GameCarRuntime *car) {
    GameCarRuntime *obj;
    s32 value;
    s32 temp;
    s32 distance;

    obj = car;
    value = 1;
    obj->motionMode = strength;
    if (strength == value) {
    } else {

    value = 2;
    if (strength == value) {
        goto angled_body_kick;
    }

    return;

    }
    value = obj->verticalMotionTimer;
    temp = 0x1E;
    obj->motionModeTimer = temp;
    value <<= 3;
        obj->motionValue.value = value;
        return;

angled_body_kick:
    value = InterpolateTrackAngle(obj->trackPointIndex);
    temp = GetAngleDistance(value, obj->bodyYaw);
    if (temp >= 0x401) {
        temp = 0x800 - temp;
    }

    distance = obj->speed;
    if (distance < 0x140) {
        obj->motionValue.value = 0;
    } else {

    value = distance - 0x140;
    value *= temp;
    if (value < 0) {
        value += 0xFFF;
    }
    value >>= 12;

    obj->motionValue.value = value;

    }
    value = 0x1E;
    obj->motionModeTimer = value;

    value = Random15();
    if (value & 0x80) {
        value = obj->motionValue.unsignedValue;
        value = -value;

        obj->motionValue.value = value;
    }
}
