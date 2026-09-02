#include "game/car.h"
#include "game/car_internal.h"

static void UpdateRivalRise(GameCarRuntime *car, s32 ground, s32 tick) {
    s32 rise = (s16)tick;

    car->y += car->verticalMotionRate * rise +
              rise * rise * CAR_JUMP_RISE_CURVE / CAR_JUMP_CURVE_SCALE;
    if (car->y >= ground) {
        car->verticalMotionState = CAR_VERTICAL_GROUNDED;
    }
}

static void UpdateRivalCrest(GameCarRuntime *car, s32 ground) {
    if (car->verticalTargetY >= ground - car->verticalMotionRate) {
        car->y = car->verticalTargetY;
    } else {
        car->verticalMotionState = CAR_VERTICAL_FALLING;
        car->verticalMotionRate = car->verticalMotionTimer;
        car->y = car->verticalTargetY;
    }
}

static void UpdateRivalFall(GameCarRuntime *car, s32 ground, s32 tick) {
    s16 fall = tick - (u16)car->verticalMotionRate;

    car->y = car->verticalTargetY +
             fall * fall * CAR_JUMP_FALL_CURVE / CAR_JUMP_CURVE_SCALE;
    if (car->y >= ground) {
        car->verticalMotionState = CAR_VERTICAL_GROUNDED;
    }
}

static void LandRivalCar(GameCarRuntime *car, s32 ground) {
    car->y = ground + CAR_WHEEL_GROUND_OFFSET;
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    StartCarBodyKick(car, CAR_BODY_KICK_LANDING);
}

static void UpdateRivalJumpArc(GameCarRuntime *car, s32 ground) {
    s32 tick = (u16)car->verticalMotionTimer + 1;

    car->verticalMotionTimer = tick;
    switch (car->verticalMotionState) {
    case CAR_VERTICAL_RISING:
        UpdateRivalRise(car, ground, tick);
        break;
    case CAR_VERTICAL_AT_CREST:
        UpdateRivalCrest(car, ground);
        break;
    case CAR_VERTICAL_FALLING:
    default:
        UpdateRivalFall(car, ground, tick);
        break;
    }

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        LandRivalCar(car, ground);
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

        ground = car->y - CAR_WHEEL_GROUND_OFFSET;
        UpdateCarWheelRotation(car);
        CopyCarBodyRotationToModel(car);
        car->bodyRoll += car->bodyRollVelocity;
        car->modelY = car->y;
        if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
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
