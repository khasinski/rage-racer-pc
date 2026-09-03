#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

enum {
    WHEEL_BLUR_SPEED = 800,
    MAX_WHEEL_ROTATION_STEP = 0x1000,
    BLURRED_WHEEL_ROTATION_STEP = 0x249,
};

void UpdateCarWheelRotation(GameCarRuntime *car) {
    s32 step = WrapSigned32((int64_t)car->speed * 3);
    s32 rotation;

    if (step > MAX_WHEEL_ROTATION_STEP) {
        step = BLURRED_WHEEL_ROTATION_STEP;
    }
    rotation = ((u32)car->wheelRotation + (u32)step) & ANGLE_MASK;
    car->wheelRotation = car->speed > WHEEL_BLUR_SPEED
        ? rotation | CAR_WHEEL_BLUR_FLAG
        : rotation;
}
