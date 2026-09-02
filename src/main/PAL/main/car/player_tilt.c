#include "game/car.h"
#include "game/race.h"

enum {
    ACTIVE_RACE_PHASE = 2,
    PEDAL_ACTIVE_THRESHOLD = 0x81,
    TILT_BRAKE_MIN_SPEED = 0x51,
    TILT_REST = 8,
};

void UpdatePlayerTilt(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    if (g_RacePhase < ACTIVE_RACE_PHASE) {
        car->tiltCounter = TILT_REST;
        return;
    }

    if (car->verticalMotionState == 0) {
        if (drive->engineRpm >= g_CarSpec->redline &&
            drive->acceleratorInput.value >= PEDAL_ACTIVE_THRESHOLD &&
            drive->clutch == 0) {
            s32 tilt = (s16)((u16)car->tiltCounter - 4);
            s32 minimum = -(9 - drive->manual) * 5;

            car->tiltCounter = (s16)(tilt < minimum ? minimum : tilt);
            return;
        }

        if ((drive->brakeInput >= PEDAL_ACTIVE_THRESHOLD ||
             drive->clutch > 0) &&
            car->speed >= TILT_BRAKE_MIN_SPEED) {
            s32 tilt = (s16)((u16)car->tiltCounter + 2);

            car->tiltCounter = (s16)(tilt >= 9 ? TILT_REST : tilt);
            return;
        }
    }

    car->tiltCounter = (s16)(car->tiltCounter * 3 / 4);
}
