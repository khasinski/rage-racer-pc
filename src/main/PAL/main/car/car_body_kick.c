#include "game/angle.h"
#include "game/car.h"
#include "game/random.h"
#include "game/track.h"

enum {
    BODY_KICK_LANDING = 1,
    BODY_KICK_CORNERING = 2,
    BODY_KICK_DURATION = 30,
    BODY_KICK_MIN_SPEED = 0x140,
};

/* Start the short body impulse produced by a landing or a fast sideways hit. */
void StartCarBodyKick(s32 strength, GameCarRuntime *car) {
    s32 lean;
    s32 speedOverMinimum;

    car->motionMode = (s16)strength;
    if (strength == BODY_KICK_LANDING) {
        car->motionModeTimer = BODY_KICK_DURATION;
        car->motionValue.value = car->verticalMotionTimer << 3;
        return;
    }
    if (strength != BODY_KICK_CORNERING) {
        return;
    }

    lean = GetAngleDistance(
        InterpolateTrackAngle(car->trackPointIndex, car->segmentFraction),
        car->bodyYaw);
    if (lean > ANGLE_QUARTER_TURN) {
        lean = ANGLE_HALF_TURN - lean;
    }

    speedOverMinimum = car->speed - BODY_KICK_MIN_SPEED;
    car->motionValue.value = speedOverMinimum < 0
                                 ? 0
                                 : speedOverMinimum * lean / ANGLE_FULL_TURN;
    car->motionModeTimer = BODY_KICK_DURATION;
    if (Random15() & 0x80) {
        car->motionValue.value = -car->motionValue.unsignedValue;
    }
}
