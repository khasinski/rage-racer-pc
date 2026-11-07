#include "game/car_motion_internal.h"
#include "game/car_shift.h"
#include "game/integer.h"

enum {
    DRIVETRAIN_RECONNECT_FRAMES = 3,
    LANDING_SOUND_LEVEL_MASK = 0x3F,
    AIRBORNE_LAUNCH_SPEED_DIVISOR = 0x100000,
};

static void ReconnectPlayerDrivetrain(PlayerCarRuntime *car, const GameCarSpec *spec) {
    GameCarDrive *drive = &car->drive;

    drive->shiftSoundLevel =
        car->verticalMotionTimer & LANDING_SOUND_LEVEL_MASK;
    drive->yawOffset = 0;
    drive->launchHeading = car->headingAngle;
    drive->launchSpeed = car->speed / AIRBORNE_LAUNCH_SPEED_DIVISOR;
    drive->spinRate = 0;
    PrepareAirborneDrivetrain(car, spec);
}

static void LandPlayerCar(PlayerCarRuntime *car, const GameCarSpec *spec,
                          s32 groundHeight) {
    GameCarDrive *drive = &car->drive;

    ApplyCarLandingPose(AsRivalCar(car), groundHeight);
    drive->shiftSoundLevel = 0;
    if (drive->motionState == CAR_MOTION_DRIVING &&
        car->verticalMotionTimer >= DRIVETRAIN_RECONNECT_FRAMES) {
        ReconnectPlayerDrivetrain(car, spec);
    }
}

int StepPlayerJump(PlayerCarRuntime *car, const GameCarSpec *spec,
                    s32 groundHeight) {
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        return 0;
    }

    AdvanceCarJumpArc(AsRivalCar(car), groundHeight);
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        LandPlayerCar(car, spec, groundHeight);
        return 1;
    }
    return 0;
}
