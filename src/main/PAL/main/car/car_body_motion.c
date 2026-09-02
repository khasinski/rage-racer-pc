#include "game/car.h"
#include "psyq/gte.h"

enum {
    BODY_KICK_INACTIVE = 0,
    BODY_KICK_LANDING = 1,
    BODY_KICK_CORNERING = 2,
};

static void StopCarBodyKick(GameCarRuntime *car) {
    car->motionMode = BODY_KICK_INACTIVE;
    car->motionModeTimer = 0;
    car->bodyKickOffset = 0;
}

/* Apply the short, fading body impulse started by a landing or side impact. */
void UpdateCarBodyKick(GameCarRuntime *car) {
    s32 value;
    s32 wave;
    s32 amplitude;
    s32 timer;

    if (car->motionMode == BODY_KICK_INACTIVE) {
        return;
    }
    if (car->motionMode != BODY_KICK_LANDING &&
        car->motionMode != BODY_KICK_CORNERING) {
        StopCarBodyKick(car);
        return;
    }
    if (car->motionModeTimer <= 1) {
        StopCarBodyKick(car);
        return;
    }

    car->motionModeTimer--;
    timer = car->motionModeTimer;
    amplitude = timer * car->motionValue.value / 128;
    wave = rsin(((timer * 3) << 12) / 30) * amplitude;
    value = wave / 2048;

    switch (car->motionMode) {
    case BODY_KICK_LANDING:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        car->bodyKickOffset = value + amplitude / 2;
        car->bodyRoll += car->bodyKickOffset / 2;
        break;
    case BODY_KICK_CORNERING:
        if (car->verticalMotionState == 0) {
            car->bodyRoll += value;
        }
        break;
    }
}
