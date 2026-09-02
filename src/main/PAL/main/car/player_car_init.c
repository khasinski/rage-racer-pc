#include "game/audio.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"

#include <stdio.h>

static void ResetPlayerCarRuntime(PlayerCarRuntime *car) {
    car->modelIndex = 0x17;
    car->drive.brakePos = 0;
    car->drive.reserved0C = 0;
    car->drive.accelPos = 0;
    car->drive.reserved20 = 0;
    car->drive.steerPos = 0;
    car->drive.reserved18 = 0;
    car->motionX = 0;
    car->motionY = 0;
    car->motionZ = 0;
    car->wheelRotation = 0;
    car->steeringAngle = 0;
    car->reserved40 = 0;
    car->speed = 0;
    car->acceleration = 0;
    car->lap = 0;
    car->drive.bodyLiftOffset = 0;
    car->progressA = 0;
    car->progressB = 0;
    car->trackProgress = 0;
}

static void PlacePlayerCarOnGrid(PlayerCarRuntime *car) {
    CarTrackLimits trackLimits = {0};
    const TrackRivalStart *start;
    s32 raceSeries = ReadStableRaceSeries();

    start = &g_TrackEventData->rivalStarts[raceSeries][0];
    car->trackPointIndex = start->trackPointIndex;
    car->x = start->x;
    car->y = 0;
    car->z = start->z;
    car->trackPointIndex =
        FindTrackSegment(AsRivalCar(car), car->trackPointIndex);

    car->bodyPitch = 0;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    raceSeries * ANGLE_HALF_TURN -
                    TrackPoint(car->trackPointIndex)->angle) & ANGLE_MASK;
    car->bodyRoll = 0;
    car->bodyRollVelocity = 0;
    car->previousTrackPointIndex = car->trackPointIndex;
    car->headingAngle = car->bodyYaw;
    car->drive.targetHeading = car->headingAngle;

    SeedCarLapProgress(GetPlayerCarRuntime(car), 0);
    UpdateCarTrackState(AsRivalCar(car), car->trackPointIndex, &trackLimits);
    car->previousTrackProgress = car->trackProgress;
    CopyPlayerBodyRotationToModel(car);
    car->modelY = car->y;

    CalculatePlayerBodyOffset(car);

    car->x += car->motionX;
    car->z += car->motionZ;
    car->facingBackwards = IsCarFacingBackwards(car);
}

static void InitializePlayerDrive(GameCarDrive *drive) {
    drive->hudLapHighlightRow = -1;
    drive->motionState = CAR_MOTION_STANDING_START;
    drive->engineLoad = 0;
    drive->drivetrainCoupled = 1;
    drive->shiftSpeedDelta = 0;
    drive->steeringGrip = 0;
    drive->trackCurveBias = 0;
    drive->trackCurveMode = 0;
    drive->jumpTimer = 0;
    drive->clutch = 0;
    drive->groundedFrames = 0;
    drive->launchEnergy = 0;
    drive->standingStartBounceY = 0;
    drive->standingStartBounceX = 0;
    drive->gear = 1;
    drive->engineRpm = 0;
    drive->reserved80 = 0;
    drive->drivetrainTorque = 0;
    drive->reserved7C = 0;
    drive->racePosition = 1;
    drive->gearDisp = 1;
    drive->shiftRpmDelta = 0;
}

static void ResetPlayerDrivingGlobals(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    car->verticalMotionState = 0;
    drive->brakeLatch = 0;
    drive->acceleratorLatch = 0;
    g_EngineRpmJitter = 0;
    g_EngineRpm = 0;
    g_EngineRpmSnapshot = 0;
    g_StandingStartSpin = 0;
    g_DriveBoostTimer = 0;
    g_HudGlyphClut = drive->manual != 0 ? 0x7800 : 0x78CF;
    g_DragScale = 0x3E8;
    g_SteerHoldFrames = 0;
    g_GripLossTimer = 0;
    g_WrongWayTimer = 0;
    g_PlayerAutoSteer = 0;
}

void InitPlayerCar(PlayerCarRuntime *car) {
    printf("%s", g_MsgInitCar);
    g_RacePhase = 2;
    g_RaceSeries = g_GrandPrixSeries & 1;
    BuildTachoNeedleQuad();
    ClearCarMotionState(AsRivalCar(car));
    g_AutoShiftCooldown = 0;
    g_TrackZoneDark = 0;
    g_ShiftSoundLevel = 0;
    g_RoadGrade = 0;

    ResetPlayerCarRuntime(car);
    printf("%s", g_MsgHTbl);
    PlacePlayerCarOnGrid(car);
    InitializePlayerDrive(&car->drive);
    g_ShiftTargetRpm = 0;

    printf("%s", g_MsgInit0);
    printf("%s", g_MsgInit1);
    PrepareCarPerformance(&car->drive);
    printf("%s", g_MsgInit5);
    ResetPlayerDrivingGlobals(car);
    printf("%s", g_MsgInit6);
    printf(g_FmtLongLine, car->progressA);
    printf("%s", g_MsgInitOk);
}
