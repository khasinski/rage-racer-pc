#include "game/angle.h"
#include "game/car.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track.h"

void SeedFinishCamera(PlayerCarRuntime *car) {
    const GameTrackPoint *point;
    s32 heading;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return;
    }

    point = TrackPoint(car->trackPointIndex);
    g_CameraCar = *AsRivalCar(car);
    g_CameraCar.x = point->x;
    g_CameraCar.z = point->z;
    g_CameraCar.y = point->y - 0x40;
    g_CameraCar.speed = WrapSigned32((int64_t)g_CameraCar.speed + 0x40);

    heading = car->facingBackwards * ANGLE_HALF_TURN +
              ANGLE_THREE_QUARTER_TURN - point->angle;
    g_CameraCar.headingAngle = heading;
    g_CameraCarSeedYaw = heading;
    g_CameraCar.bodyYaw = heading;
}
