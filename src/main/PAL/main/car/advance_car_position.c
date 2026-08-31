#include "game/car.h"
#include "game/render.h"

/*
 * Turns a car's travel direction towards the way its body points, and gives
 * it the part of its acceleration that pushes it along.
 *
 * Two directions matter and they are not the same one. `headingAngle` is the
 * way the car is travelling; `bodyYaw` is the way it is facing. Acceleration
 * acts along the body, so a car whose nose is off its line is pushed sideways
 * as well as forwards.
 *
 * That gives both answers here. Adding the push to the current motion yields
 * a new direction of travel, which becomes the heading. The speed gains only
 * the component of the push that lies along the direction the car was already
 * going, which is why a car sliding sideways gains little from the throttle.
 *
 * Despite what its name suggests, nothing here moves the car: the caller does
 * that with the heading and speed this leaves behind.
 */
void AdvanceCarPosition(GameCarRuntime *car) {
    s32 headingSin = rsin(car->headingAngle);
    s32 headingCos = rcos(car->headingAngle);
    s32 bodySin = rsin(car->bodyYaw);
    s32 bodyCos = rcos(car->bodyYaw);
    s32 motionX;
    s32 motionZ;
    s32 pushAlongHeading;

    /*
     * The two are summed at a fixed ratio, and the divisions are kept as they
     * were: every one of them truncates, and the truncation is part of the
     * angle that comes out. Only the direction of this vector is used, so the
     * common scale is free, but the rounding is not.
     */
    motionX = ((headingSin * car->speed) / 4 + bodySin * car->acceleration) / 100;
    motionZ = ((headingCos * car->speed) / 4 + bodyCos * car->acceleration) / 100;

    /*
     * The cosine of the angle between travel and facing, as the dot product of
     * the two unit vectors. Written out rather than as rcos of the difference:
     * the table would round the angle first and answer a slightly different
     * question.
     */
    pushAlongHeading = (headingSin * bodySin + headingCos * bodyCos) / 4096;
    car->speed += (pushAlongHeading * car->acceleration) / 4096;

    car->headingAngle = ANGLE_QUARTER_TURN - Atan2(motionX, motionZ);
}
