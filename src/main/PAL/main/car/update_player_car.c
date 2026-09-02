#include "game/state.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/random.h"

#include "rage/trace.h"

enum {
    PLAYER_BODY_GROUND_OFFSET = 8,
    PEDAL_POSITION_SCALE = 6,
    PEDAL_POSITION_DIVISOR = 1280,
    RANDOM15_MAX = 0x7FFF,
    SHIFT_PITCH_SCALE = 100,
};

static void IntegratePlayerHorizontalPosition(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    car->x -= car->motionX;
    car->z -= car->motionZ;
    CalculatePlayerBodyOffset(car);
    car->x += car->motionX +
              drive->accelPos * PEDAL_POSITION_SCALE / PEDAL_POSITION_DIVISOR;
    car->z += car->motionZ +
              drive->brakePos * PEDAL_POSITION_SCALE / PEDAL_POSITION_DIVISOR;
}

static void ApplyGearShiftBodyPitch(PlayerCarRuntime *car) {
    s32 rpmSurplus;

    if (car->drive.shiftRpmDelta == 0) return;

    rpmSurplus = (g_CarSpec->revLimit + g_CarSpec->redline) / 2 -
                 g_ShiftTargetRpm;
    if (rpmSurplus > 0) {
        car->bodyPitch +=
            rpmSurplus * Random15() / (SHIFT_PITCH_SCALE * RANDOM15_MAX);
    }
}

/* Per-frame player physics orchestration and track contact. */
void UpdatePlayerCar(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 useAlternateGearMapping;
    s32 groundHeight;
    s32 skid;
    s32 crash;

    TraceCarStates();

    useAlternateGearMapping = g_PadType == PAD_TYPE_NEGCON;
    car->facingBackwards = IsCarFacingBackwards(car);

    ShiftPlayerGears(car, useAlternateGearMapping);

    UpdateCarBodyRoll(car);

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        UpdatePlayerSteeringTarget(car);
    }

    ReadPlayerCarInput(drive);
    UpdateCarDrivetrain(car);

    UpdatePlayerControlFeedback(car);

    TraceCarMotion("pre-integrate", car);
    IntegratePlayerHorizontalPosition(car);
    TraceCarMotion("post-position", car);
    AccumulateLapProgress(AsRivalCar(car));
    TraceCarMotion("post-progress", car);

    skid = ResolvePlayerTrackContact(car);

    ApplyGearShiftBodyPitch(car);

    crash = CollidePlayerWithCars(car);
    TraceCarMotion(crash != 0 ? "post-cars-hit" : "post-cars-clear", car);
    if (skid != 0 || crash != 0) {
        StartCarBodyKick(AsRivalCar(car), CAR_BODY_KICK_CORNERING);
    }

    CopyPlayerBodyRotationToModel(car);
    car->bodyRoll += car->bodyRollVelocity;
    car->modelY = car->y;
    groundHeight = car->y - PLAYER_BODY_GROUND_OFFSET;

    UpdatePlayerJump(car, groundHeight);

    UpdatePlayerTilt(car);
    UpdateCarCrestHop(AsRivalCar(car));

    ApplyPlayerContactResponse(car, skid, crash);

    UpdatePlayerEnginePresentation(car);
}
