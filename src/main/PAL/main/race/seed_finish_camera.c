#include "game/angle.h"
#include "game/car.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track.h"

void SeedFinishCamera(FinishCamera *finish, PlayerCarRuntime *car) {
    const GameTrackPoint *point;
    s32 heading;

    if (car == NULL || g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return;
    }

    point = TrackPoint(car->trackPointIndex);
    finish->car = *AsRivalCar(car);
    finish->car.x = point->x;
    finish->car.z = point->z;
    finish->car.y = point->y - 0x40;
    finish->car.speed = WrapSigned32((int64_t)finish->car.speed + 0x40);

    heading = (car->facingBackwards != 0 ? ANGLE_HALF_TURN : 0) +
              ANGLE_THREE_QUARTER_TURN - point->angle;
    finish->car.headingAngle = heading;
    finish->seedYaw = heading;
    finish->car.bodyYaw = heading;
    finish->point = car->trackPointIndex;
    finish->heading = heading;
    finish->section = car->trackSection;
}
