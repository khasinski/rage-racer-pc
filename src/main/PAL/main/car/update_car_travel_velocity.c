#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/render.h"

enum {
    TRAVEL_SPEED_SCALE = 4,
    TRAVEL_VECTOR_SCALE = 100,
    TRIG_SCALE = 4096,
};

/* Keep the two axis calculations identical. The divisions cannot be combined:
 * retail truncates the existing-speed contribution before adding acceleration. */
static s32 CalculateTravelAxis(s32 headingDirection, s32 bodyDirection,
                               s32 speed, s32 acceleration) {
    const s32 carriedSpeed =
        WrapSigned32((int64_t)headingDirection * speed) /
        TRAVEL_SPEED_SCALE;
    const s32 accelerationPush =
        WrapSigned32((int64_t)bodyDirection * acceleration);

    return WrapSigned32((int64_t)carriedSpeed + accelerationPush) /
           TRAVEL_VECTOR_SCALE;
}

/*
 * Updates a car's travel direction and speed from the acceleration applied in
 * the direction its body faces. `headingAngle` is the direction of travel;
 * `bodyYaw` is the direction of that acceleration, so a sliding car gains
 * only the component that lies along its existing travel direction.
 */
void UpdateCarTravelVelocity(GameCarRuntime *car) {
    const s32 headingSin = rsin(car->headingAngle);
    const s32 headingCos = rcos(car->headingAngle);
    const s32 bodySin = rsin(car->bodyYaw);
    const s32 bodyCos = rcos(car->bodyYaw);
    const s32 motionX = CalculateTravelAxis(
        headingSin, bodySin, car->speed, car->acceleration);
    const s32 motionZ = CalculateTravelAxis(
        headingCos, bodyCos, car->speed, car->acceleration);
    const s32 pushAlongHeading = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)headingSin * bodySin) +
        WrapSigned32((int64_t)headingCos * bodyCos)) / TRIG_SCALE;

    car->speed = WrapSigned32(
        (int64_t)car->speed +
        WrapSigned32(
            (int64_t)pushAlongHeading * car->acceleration) / TRIG_SCALE);

    if (motionX != 0 || motionZ != 0) {
        car->headingAngle = ANGLE_QUARTER_TURN - Atan2(motionX, motionZ);
    }
}
