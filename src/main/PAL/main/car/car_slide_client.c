#include "game/car_motion_internal.h"
#include "game/race.h"

void UpdateCarSlideAngle(GameCarRuntime *car, s32 slideScale) {
    StepCarSlide(car, slideScale, g_RaceSeries != 0);
}
