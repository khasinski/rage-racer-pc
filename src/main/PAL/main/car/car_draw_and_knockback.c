#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/random.h"
void DrawCars(void) {
    GameCarRuntime *base;
    s32 i;

    base = g_Cars;
    SelectModelBank(1);

    for (i = 0; i < 11; i++) {
        if (base->activeFlag != -1) {
            if (base->aiEnabled == 1) {
                DrawCar(GetCarRenderObject(base));
            }
        }
        base++;
    }
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
