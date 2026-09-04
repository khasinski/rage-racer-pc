#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"

enum {
    LONG_LANDING_SOUND_FRAMES = 19,
    DRIVETRAIN_RECONNECT_FRAMES = 3,
    LANDING_SOUND_LEVEL_MASK = 0x3F,
    AIRBORNE_LAUNCH_SPEED_DIVISOR = 0x100000,
    LANDING_SOUND_CUE = 0xE,
};

static void ReconnectPlayerDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    g_ShiftSoundLevel =
        car->verticalMotionTimer & LANDING_SOUND_LEVEL_MASK;
    drive->yawOffset = 0;
    drive->launchHeading = car->headingAngle;
    drive->launchSpeed = car->speed / AIRBORNE_LAUNCH_SPEED_DIVISOR;
    drive->spinRate = 0;
    PrepareAirborneDrivetrain(car);
}

static void LandPlayerCar(PlayerCarRuntime *car, s32 groundHeight) {
    GameCarDrive *drive = &car->drive;

    ApplyCarLandingPose(AsRivalCar(car), groundHeight);
    g_ShiftSoundLevel = 0;
    if (car->verticalMotionTimer >= LONG_LANDING_SOUND_FRAMES &&
        g_RacePhase <= RACE_PHASE_ACTIVE) {
        PlaySoundCue(LANDING_SOUND_CUE);
    }
    if (drive->motionState == CAR_MOTION_DRIVING &&
        car->verticalMotionTimer >= DRIVETRAIN_RECONNECT_FRAMES) {
        ReconnectPlayerDrivetrain(car);
    }
}

void UpdatePlayerJump(PlayerCarRuntime *car, s32 groundHeight) {
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        return;
    }

    AdvanceCarJumpArc(AsRivalCar(car), groundHeight);
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        LandPlayerCar(car, groundHeight);
    }
}
