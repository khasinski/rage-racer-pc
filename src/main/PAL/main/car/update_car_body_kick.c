#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "psyq/gte.h"

enum {
    BODY_KICK_WAVE_CYCLES = 3,
    BODY_KICK_AMPLITUDE_SCALE = 128,
    BODY_KICK_WAVE_SCALE = 2048,
};

static void StopCarBodyKick(GameCarRuntime *car) {
    car->motionMode = CAR_BODY_KICK_INACTIVE;
    car->motionModeTimer = 0;
    car->bodyKickOffset = 0;
}

/* Apply the short, fading body impulse started by a landing or side impact. */
void UpdateCarBodyKick(GameCarRuntime *car) {
    s32 value;
    s32 wave;
    s32 amplitude;
    s32 timer;

    if (car->motionMode == CAR_BODY_KICK_INACTIVE) {
        return;
    }
    if (car->motionMode != CAR_BODY_KICK_LANDING &&
        car->motionMode != CAR_BODY_KICK_CORNERING) {
        StopCarBodyKick(car);
        return;
    }
    if (car->motionModeTimer <= 1) {
        StopCarBodyKick(car);
        return;
    }

    car->motionModeTimer--;
    timer = car->motionModeTimer;
    amplitude = timer * car->motionValue.value / BODY_KICK_AMPLITUDE_SCALE;
    wave = WrapSigned32(
        (int64_t)rsin(((timer * BODY_KICK_WAVE_CYCLES) << 12) /
                     CAR_BODY_KICK_DURATION) * amplitude);
    value = wave / BODY_KICK_WAVE_SCALE;

    switch (car->motionMode) {
    case CAR_BODY_KICK_LANDING:
        car->bodyKickOffset = WrapSigned16(value + amplitude);
        car->bodyPitch = WrapSigned16(
            (s32)car->bodyPitch + car->bodyKickOffset);
        car->bodyKickOffset = WrapSigned16(value + amplitude / 2);
        car->bodyRoll = WrapSigned16(
            (s32)car->bodyRoll + car->bodyKickOffset / 2);
        break;
    case CAR_BODY_KICK_CORNERING:
        if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
            car->bodyRoll = WrapSigned16((s32)car->bodyRoll + value);
        }
        break;
    }
}
