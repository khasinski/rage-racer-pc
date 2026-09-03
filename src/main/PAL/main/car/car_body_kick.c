#include "game/angle.h"
#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/random.h"
#include "game/track.h"

enum {
    BODY_KICK_MIN_SPEED = 0x140,
};

/* Start the short body impulse produced by a landing or a fast sideways hit. */
void StartCarBodyKick(GameCarRuntime *car, CarBodyKickMode mode) {
    s32 lean;
    s32 speedOverMinimum;

    if (mode != CAR_BODY_KICK_LANDING && mode != CAR_BODY_KICK_CORNERING) {
        return;
    }
    car->motionMode = mode;
    if (mode == CAR_BODY_KICK_LANDING) {
        car->motionModeTimer = CAR_BODY_KICK_DURATION;
        car->motionValue.value = WrapSigned16(
            car->verticalMotionTimer * 8);
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
        : WrapSigned16(
              WrapSigned32((int64_t)speedOverMinimum * lean) /
              ANGLE_FULL_TURN);
    car->motionModeTimer = CAR_BODY_KICK_DURATION;
    if (Random15() & 0x80) {
        car->motionValue.value = WrapSigned16(
            -(s32)car->motionValue.unsignedValue);
    }
}
