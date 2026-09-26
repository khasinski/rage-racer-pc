#include "game/car_motion_internal.h"
#include "game/integer.h"

enum { PEDAL_POSITION_SCALE = 6, PEDAL_POSITION_DIVISOR = 1280 };

void IntegratePlayerPosition(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    car->x = WrapSigned32((int64_t)car->x - car->motionX);
    car->z = WrapSigned32((int64_t)car->z - car->motionZ);
    CalculatePlayerBodyOffset(car);
    car->x = WrapSigned32((int64_t)car->x + car->motionX);
    car->x = WrapSigned32(
        (int64_t)car->x +
        WrapSigned32((int64_t)drive->accelPos * PEDAL_POSITION_SCALE) /
            PEDAL_POSITION_DIVISOR);
    car->z = WrapSigned32((int64_t)car->z + car->motionZ);
    car->z = WrapSigned32(
        (int64_t)car->z +
        WrapSigned32((int64_t)drive->brakePos * PEDAL_POSITION_SCALE) /
            PEDAL_POSITION_DIVISOR);
}

