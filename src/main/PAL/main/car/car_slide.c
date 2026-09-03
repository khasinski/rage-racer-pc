#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/race.h"

enum {
    SLIDE_START_SPEED = 0x3C1,
    SLIDE_INPUT_SPEED_SCALE = 0x320,
    MAX_SLIDE_YAW_RATE = 0x2BC,
};

/* Ease a slide back towards straight and clear its direction at rest. */
static void SettleSlide(GameCarRuntime *car) {
    s32 rate = car->yawRate;

    if (rate == 0) {
        return;
    }
    rate = WrapSigned32((int64_t)rate * 15) / 16;
    car->yawRate = rate;
    if (rate == 0) {
        car->slideActive = 0;
    }
}

void UpdateCarSlideAngle(GameCarRuntime *car, s32 slideScale) {
    s32 input;
    s32 rate;

    if (car->slideInput.value == 0) {
        if (car->yawRate == 0 && slideScale != 0) {
            if (car->speed < SLIDE_START_SPEED) {
                return;
            }
            input = WrapSigned32((int64_t)slideScale * car->speed) /
                    SLIDE_INPUT_SPEED_SCALE;
            car->slideInput.value = g_RaceSeries != 0
                                        ? WrapSigned32(-(int64_t)input)
                                        : input;
            car->yawRate = 0;
            return;
        }
        SettleSlide(car);
        return;
    }

    input = WrapSigned32((int64_t)car->slideInput.value * 31) / 32;
    car->slideInput.value = input;
    rate = WrapSigned32((int64_t)car->yawRate - input / 2);
    if (rate > MAX_SLIDE_YAW_RATE) {
        rate = MAX_SLIDE_YAW_RATE;
    } else if (rate < -MAX_SLIDE_YAW_RATE) {
        rate = -MAX_SLIDE_YAW_RATE;
    }
    car->yawRate = rate;
}
