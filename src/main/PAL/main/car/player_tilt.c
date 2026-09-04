#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/race.h"

enum {
    PEDAL_ACTIVE_THRESHOLD = 0x81,
    TILT_BRAKE_MIN_SPEED = 0x51,
    TILT_REST = 8,
    ACCELERATION_TILT_STEP = 4,
    BRAKING_TILT_STEP = 2,
    BRAKING_TILT_RESET_THRESHOLD = 9,
    ACCELERATION_TILT_LIMIT_UNITS = 9,
    MANUAL_TILT_LIMIT_REDUCTION = 1,
    ACCELERATION_TILT_UNIT = 5,
    TILT_DAMPING_NUMERATOR = 3,
    TILT_DAMPING_DENOMINATOR = 4,
};

void UpdatePlayerTilt(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    if (g_RacePhase < RACE_PHASE_ACTIVE) {
        car->tiltCounter = TILT_REST;
        return;
    }

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        if (drive->engineRpm >= g_CarSpec->redline &&
            drive->acceleratorInput.value >= PEDAL_ACTIVE_THRESHOLD &&
            drive->clutch == 0) {
            s32 tilt = WrapSigned16(
                (s32)car->tiltCounter - ACCELERATION_TILT_STEP);
            s32 minimum =
                -(ACCELERATION_TILT_LIMIT_UNITS -
                  drive->manual * MANUAL_TILT_LIMIT_REDUCTION) *
                ACCELERATION_TILT_UNIT;

            car->tiltCounter = WrapSigned16(
                tilt < minimum ? minimum : tilt);
            return;
        }

        if ((drive->brakeInput >= PEDAL_ACTIVE_THRESHOLD ||
             drive->clutch > 0) &&
            car->speed >= TILT_BRAKE_MIN_SPEED) {
            s32 tilt = WrapSigned16(
                (s32)car->tiltCounter + BRAKING_TILT_STEP);

            car->tiltCounter = WrapSigned16(
                tilt >= BRAKING_TILT_RESET_THRESHOLD ? TILT_REST : tilt);
            return;
        }
    }

    car->tiltCounter =
        car->tiltCounter * TILT_DAMPING_NUMERATOR /
        TILT_DAMPING_DENOMINATOR;
}
