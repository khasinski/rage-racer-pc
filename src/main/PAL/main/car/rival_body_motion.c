#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

static void LandRivalCar(GameCarRuntime *car, s32 ground) {
    car->y = WrapSigned32(
        (int64_t)ground + CAR_WHEEL_GROUND_OFFSET);
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    StartCarBodyKick(car, CAR_BODY_KICK_LANDING);
}

void UpdateRivalBodyMotion(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        s32 ground;

        if (car->activeFlag == -1) {
            continue;
        }

        ground = WrapSigned32(
            (int64_t)car->y - CAR_WHEEL_GROUND_OFFSET);
        UpdateCarWheelRotation(car);
        CopyCarBodyRotationToModel(car);
        car->bodyRoll = WrapSigned32(
            (int64_t)car->bodyRoll + car->bodyRollVelocity);
        car->modelY = car->y;
        if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
            AdvanceCarJumpArc(car, ground);
            if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
                LandRivalCar(car, ground);
            }
        }
        if (car->collisionFlag == 0) {
            UpdateCarBodyKick(car);
            UpdateCarCrestHop(car);
        } else {
            car->speed = WrapSigned32((int64_t)car->speed * 97) / 100;
            car->speed = WrapSigned32((int64_t)car->speed * 97) / 100;
        }
    }
}
