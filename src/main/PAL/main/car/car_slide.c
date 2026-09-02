#include "game/car.h"
#include "game/race.h"

/* Ease a slide back towards straight and clear its direction at rest. */
static void SettleSlide(GameCarAiBlock *ai) {
    s32 rate = ai->yawRate;

    if (rate == 0) return;
    rate = rate * 15 / 16;
    ai->yawRate = rate;
    if (rate == 0) ai->markerDirection = 0;
}

void UpdateCarSlideAngle(GameCarRuntime *car, s32 carIndex) {
    GameCarAiBlock *ai = GetCarAiBlock(car);
    s32 adjusted;
    s32 input;
    s32 rate;

    if (car->slideInput.value == 0) {
        if (car->yawRate == 0 && carIndex != 0) {
            if (car->speed < 0x3C1) return;
            input = carIndex * car->speed / 0x320;
            car->slideInput.value = g_RaceSeries != 0 ? -input : input;
            car->yawRate = 0;
            return;
        }
        if (ai->slideInput.value == 0) {
            SettleSlide(ai);
            return;
        }
    }

    /* Decay the input by 31/32 towards zero. The sign bit preserves retail's
     * rounding when half of the remaining input is removed from yaw. */
    adjusted = ai->slideInput.value * 31;
    if (adjusted < 0) adjusted += 0x1F;
    input = adjusted >> 5;
    ai->slideInput.value = input;
    rate = ai->yawRate - ((input + (s32)((u32)adjusted >> 31)) >> 1);
    if (rate > 0x2BB)
        rate = 0x2BC;
    else if (rate < -0x2BB)
        rate = -0x2BC;
    ai->yawRate = rate;
}
