#include "game/audio.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"

#include <string.h>

enum {
    PLAYER_RENDER_MODEL_INDEX = 0x17,
    INITIAL_RACE_POSITION = 1,
    RACE_DIRECTION_BIT = 1,
    MANUAL_HUD_GLYPH_CLUT = 0x7800,
    AUTOMATIC_HUD_GLYPH_CLUT = 0x78CF,
    DEFAULT_DRAG_SCALE = 1000,
};

static void ResetPlayerCarRuntime(PlayerCarRuntime *car) {
    s16 manual = car->drive.manual;
    s32 launchThresholdIndex = car->drive.launchThresholdIndex;

    memset(car, 0, sizeof(*car));
    car->modelIndex = PLAYER_RENDER_MODEL_INDEX;
    car->drive.manual = manual;
    car->drive.launchThresholdIndex = launchThresholdIndex;
    car->drive.hudLapHighlightRow = -1;
    car->drive.motionState = CAR_MOTION_STANDING_START;
    car->drive.drivetrainCoupled = 1;
    car->drive.gear = CAR_FIRST_FORWARD_GEAR;
    car->drive.racePosition = INITIAL_RACE_POSITION;
    car->drive.gearDisp = CAR_FIRST_FORWARD_GEAR;
}

static void PlacePlayerCarOnGrid(PlayerCarRuntime *car) {
    CarTrackLimits trackLimits = {0};
    const TrackRivalStart *start;
    s32 raceSeries = g_RaceSeries;
    s32 startPointIndex;

    if (g_TrackEventData == NULL || g_TrackPoints == NULL ||
        g_TrackPointCount <= 0) {
        return;
    }

    start = &g_TrackEventData->rivalStarts[raceSeries][0];
    startPointIndex = WrapTrackPointIndex(start->trackPointIndex);
    car->trackPointIndex = startPointIndex;
    car->x = start->x;
    car->y = 0;
    car->z = start->z;
    car->trackPointIndex =
        FindTrackSegment(AsRivalCar(car), car->trackPointIndex);
    if (car->trackPointIndex < 0) {
        car->trackPointIndex = startPointIndex;
        car->x = TrackPoint(startPointIndex)->x;
        car->z = TrackPoint(startPointIndex)->z;
    }

    car->bodyPitch = 0;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    raceSeries * ANGLE_HALF_TURN -
                    TrackPoint(car->trackPointIndex)->angle) & ANGLE_MASK;
    car->bodyRoll = 0;
    car->bodyRollVelocity = 0;
    car->previousTrackPointIndex = car->trackPointIndex;
    car->headingAngle = car->bodyYaw;
    car->drive.targetHeading = car->headingAngle;

    SeedCarLapProgress(AsRivalCar(car), 0);
    UpdateCarTrackState(AsRivalCar(car), car->trackPointIndex, &trackLimits);
    car->previousTrackProgress = car->trackProgress;
    CopyPlayerBodyRotationToModel(car);
    car->modelY = car->y;

    CalculatePlayerBodyOffset(car);

    car->x = WrapSigned32((int64_t)car->x + car->motionX);
    car->z = WrapSigned32((int64_t)car->z + car->motionZ);
    car->facingBackwards = IsCarFacingBackwards(car);
}

static void ResetPlayerDrivingGlobals(const GameCarDrive *drive) {
    g_EngineRpmJitter = 0;
    g_EngineRpm = 0;
    g_EngineRpmSnapshot = 0;
    g_StandingStartSpin = 0;
    g_DriveBoostTimer = 0;
    g_HudGlyphClut = drive->manual != 0
        ? MANUAL_HUD_GLYPH_CLUT
        : AUTOMATIC_HUD_GLYPH_CLUT;
    g_DragScale = DEFAULT_DRAG_SCALE;
    g_SteerHoldFrames = 0;
    g_GripLossTimer = 0;
    g_WrongWayTimer = 0;
    g_PlayerAutoSteer = 0;
}

void InitPlayerCar(PlayerCarRuntime *car) {
    g_RacePhase = RACE_PHASE_ACTIVE;
    g_RaceSeries = g_GrandPrixSeries & RACE_DIRECTION_BIT;
    BuildTachoNeedleQuad();
    g_AutoShiftCooldown = 0;
    g_TrackZoneDark = 0;
    g_ShiftSoundLevel = 0;
    g_RoadGrade = 0;

    ResetPlayerCarRuntime(car);
    PlacePlayerCarOnGrid(car);
    g_ShiftTargetRpm = 0;

    PrepareCarPerformance(&car->drive);
    ResetPlayerDrivingGlobals(&car->drive);
}
