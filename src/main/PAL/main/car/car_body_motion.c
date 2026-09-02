#include "game/car.h"
#include "psyq/gte.h"

/* Apply the short, fading body impulse started by a landing or side impact. */
void UpdateCarBodyKick(GameCarRuntime *car) {
    s32 value;
    s32 wave;
    s32 amplitude;
    s32 timer;

    if (car->motionMode == 0) return;

    car->motionModeTimer--;
    if ((s16)car->motionModeTimer == 0) {
        car->motionMode = 0;
        car->bodyKickOffset = 0;
    }

    timer = car->motionModeTimer;
    amplitude = timer * car->motionValue.value / 128;
    wave = rsin(((timer * 3) << 12) / 30) * amplitude;
    value = wave / 2048;

    switch (car->motionMode) {
    case 1:
    case 5:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        car->bodyKickOffset = value + amplitude / 2;
        car->bodyRoll += car->bodyKickOffset / 2;
        break;
    case 2:
        if (car->verticalMotionState == 0) car->bodyRoll += value;
        break;
    case 3:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        break;
    case 4:
        car->bodyRoll += value;
        break;
    }
}
