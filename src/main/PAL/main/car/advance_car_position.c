#include "game/car.h"
#include "game/render.h"


/*
 * Shared "advance car position/heading" helper. Integrates the car's speed
 * longitudinal speed and acceleration along the car's body yaw into
 * the world position, then recomputes headingAngle. Called by each of the
 * motionState motion handlers. Register pins and the single-param/two-arg call
 * mismatch are deliberate to match; do not "fix".
 */
void AdvanceCarPosition(GameCarRuntime *car) {
    volatile s32 coords[3];

    {
        s32 angleSin;
        s32 otherSin;

        angleSin = rsin(car->headingAngle);
        otherSin = rsin(car->bodyYaw);
        coords[0] = (((angleSin * car->speed) / 4) + (otherSin * car->acceleration)) / 100;
    }

    {
        s32 angleCos;
        s32 otherCos;

        angleCos = rcos(car->headingAngle);
        otherCos = rcos(car->bodyYaw);
        coords[2] = (((angleCos * car->speed) / 4) + (otherCos * car->acceleration)) / 100;
    }

    {
        s32 angleSin;
        s32 otherSin;
        s32 angleCos;
        s32 otherCos;

        angleSin = rsin(car->headingAngle);
        otherSin = rsin(car->bodyYaw);
        angleCos = rcos(car->headingAngle);
        otherCos = rcos(car->bodyYaw);

        car->speed += ((((angleSin * otherSin) + (angleCos * otherCos)) / 4096) * car->acceleration) / 4096;
    }
    car->headingAngle = ANGLE_QUARTER_TURN - Atan2(coords[0], coords[2]);
}
