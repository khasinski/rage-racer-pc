#include "game/diagnostics.h"
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
    RaceGridSlot *table;
    s32 i;
    s16 *flagPtr;
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
    s32 minus_one;

    base = g_Cars;
    SelectModelBank(1);

    i = 0;
    minus_one = -1;
    do {
        if (base->activeFlag != (i++, minus_one)) {
            if (base->aiEnabled == 1) {
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
    s32 limit;

    obj = car;

    state = GetCarAiBlock(obj);
    if (g_RacePhase < 2) {
        value = 8;
    } else {
    if (obj->verticalMotionState == 0) {
        if (obj->engineRpm >= g_CarSpec->redline &&
            obj->acceleratorInput >= 0x81 &&
            obj->slideInput.halves.low == 0) {
            s32 tilt;

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
    value /= 4;
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

/*
 * A knock to the body. Strength one is the straight drop a landing gives, and
 * takes its size from how long the car was in the air. Anything else leans the
 * body along the track: how far the car is turned away from the road under it,
 * scaled by how fast it is going, and thrown to one side or the other at
 * random. Strengths other than one and two do nothing.
 */
void StartCarBodyKick(s32 strength, GameCarRuntime *car) {
    s32 lean;
    s32 speedOverWalking;

    car->motionMode = strength;
    if (strength == 1) {
        car->motionModeTimer = 0x1E;
        car->motionValue.value = car->verticalMotionTimer << 3;
        return;
    }
    if (strength != 2) {
        return;
    }

    /* The blend weight was missing here, so the angle came out mixed by
     * whatever happened to sit in the second argument slot. Every other
     * caller passes the car's own position between the two points. */
    lean = GetAngleDistance(
        InterpolateTrackAngle(car->trackPointIndex, car->segmentFraction),
        car->bodyYaw);
    if (lean >= 0x401) {
        lean = 0x800 - lean;
    }

    speedOverWalking = car->speed - 0x140;
    if (speedOverWalking < 0) {
        car->motionValue.value = 0;
    } else {
        car->motionValue.value = (speedOverWalking * lean) / 4096;
    }
    car->motionModeTimer = 0x1E;
    if (Random15() & 0x80) {
        car->motionValue.value = -car->motionValue.unsignedValue;
    }
}
