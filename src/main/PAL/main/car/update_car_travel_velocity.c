#include "game/car.h"
#include "game/integer.h"
#include "game/render.h"

/*
 * Updates a car's travel direction and speed from the acceleration applied in
 * the direction its body faces. `headingAngle` is the direction of travel;
 * `bodyYaw` is the direction of that acceleration, so a sliding car gains
 * only the component that lies along its existing travel direction.
 */
void UpdateCarTravelVelocity(GameCarRuntime *car) {
    s32 headingSin = rsin(car->headingAngle);
    s32 headingCos = rcos(car->headingAngle);
    s32 bodySin = rsin(car->bodyYaw);
    s32 bodyCos = rcos(car->bodyYaw);
    s32 motionX;
    s32 motionZ;
    s32 pushAlongHeading;

    /* Preserve retail's staged divisions: their integer truncation affects
     * the direction of the resulting vector. */
    motionX = WrapSigned32(
        (int64_t)(WrapSigned32(
            (int64_t)headingSin * car->speed) / 4) +
        WrapSigned32((int64_t)bodySin * car->acceleration)) / 100;
    motionZ = WrapSigned32(
        (int64_t)(WrapSigned32(
            (int64_t)headingCos * car->speed) / 4) +
        WrapSigned32((int64_t)bodyCos * car->acceleration)) / 100;

    pushAlongHeading = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)headingSin * bodySin) +
        WrapSigned32((int64_t)headingCos * bodyCos)) / 4096;
    car->speed = WrapSigned32(
        (int64_t)car->speed +
        WrapSigned32(
            (int64_t)pushAlongHeading * car->acceleration) / 4096);

    if (motionX != 0 || motionZ != 0) {
        car->headingAngle = ANGLE_QUARTER_TURN - Atan2(motionX, motionZ);
    }
}
