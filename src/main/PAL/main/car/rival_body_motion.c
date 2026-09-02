#include "game/car.h"
#include "game/car_internal.h"

enum RivalVerticalMotionState {
    RIVAL_VERTICAL_RISING = 1,
    RIVAL_VERTICAL_AT_CREST = 2,
    RIVAL_VERTICAL_FALLING = 3,
};

static void UpdateRivalJumpArc(GameCarRuntime *car, s32 ground) {
    s32 tick = (u16)car->verticalMotionTimer + 1;

    car->verticalMotionTimer = tick;
    if (car->verticalMotionState == RIVAL_VERTICAL_RISING) {
        s32 rise = (s16)tick;

        car->y += car->verticalMotionRate * rise + rise * rise * 72 / 100;
        if (car->y >= ground) {
            car->verticalMotionState = 0;
        }
    } else if (car->verticalMotionState == RIVAL_VERTICAL_AT_CREST) {
        if (car->verticalTargetY >= ground - car->verticalMotionRate) {
            car->y = car->verticalTargetY;
        } else {
            car->verticalMotionState = RIVAL_VERTICAL_FALLING;
            car->verticalMotionRate = car->verticalMotionTimer;
            car->y = car->verticalTargetY;
        }
    } else {
        s16 fall = tick - (u16)car->verticalMotionRate;

        car->y = car->verticalTargetY + fall * fall * 216 / 100;
        if (car->y >= ground) {
            car->verticalMotionState = 0;
        }
    }

    if (car->verticalMotionState == 0) {
        car->y = ground + 8;
        car->verticalPitch = 0;
        car->verticalRoll = 0;
        StartCarBodyKick(1, car);
    }
}

void UpdateRivalBodyMotion(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        s32 ground;

        if (car->activeFlag == -1) {
            continue;
        }

        ground = car->y - 8;
        UpdateCarWheelRotation(car);
        CopyCarBodyRotationToModel(car);
        car->bodyRoll += car->bodyRollVelocity;
        car->modelY = car->y;
        if (car->verticalMotionState != 0) {
            UpdateRivalJumpArc(car, ground);
        }
        if (car->collisionFlag == 0) {
            UpdateCarBodyKick(car);
            UpdateCarCrestHop(car);
        } else {
            car->speed = car->speed * 97 / 100 * 97 / 100;
        }
    }
}
