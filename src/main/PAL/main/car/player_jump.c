#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"

enum {
    LONG_LANDING_SOUND_FRAMES = 19,
    DRIVETRAIN_RECONNECT_FRAMES = 3,
    LANDING_SOUND_CUE = 0xE,
};

static void ReconnectPlayerDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    g_ShiftSoundLevel = car->verticalMotionTimer & 0x3F;
    drive->yawOffset = 0;
    drive->launchHeading = car->headingAngle;
    drive->launchSpeed = car->speed / 0x100000;
    drive->spinRate = 0;
    PrepareAirborneDrivetrain(car);
}

static void LandPlayerCar(PlayerCarRuntime *car, s32 groundHeight) {
    GameCarDrive *drive = &car->drive;

    car->y = groundHeight + CAR_WHEEL_GROUND_OFFSET;
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    StartCarBodyKick(AsRivalCar(car), CAR_BODY_KICK_LANDING);
    g_ShiftSoundLevel = 0;
    if (car->verticalMotionTimer >= LONG_LANDING_SOUND_FRAMES &&
        g_RacePhase < 3) {
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
