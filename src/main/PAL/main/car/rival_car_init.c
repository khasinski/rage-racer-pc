#include "game/angle.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"
#include "game/track_internal.h"

static s32 NormalizeTrackPointIndex(s32 index) {
    index %= g_TrackPointCount;
    return index < 0 ? index + g_TrackPointCount : index;
}

void InitRivalCar(GameCarRuntime *car,
                  s32 gridPosition,
                  RaceGridSlot *grid) {
    const TrackRivalStart *start =
        &g_TrackEventData->rivalStarts[g_RaceSeries][gridPosition + 1];
    CarTrackLimits trackLimits = {
        .rightInset = 20,
        .leftInset = -20,
    };
    s32 trackPointIndex;
    s32 startPointIndex;

    car->initializedFlag = 1;
    car->collisionFlag = 0;
    car->aiEnabled = 1;
    car->modelIndex = grid[gridPosition].halves.modelId;
    car->rivalModelId = grid[gridPosition].halves.modelId;
    startPointIndex = NormalizeTrackPointIndex(start->trackPointIndex);
    car->trackPointIndex = startPointIndex;
    car->x = start->x;
    car->z = start->z;
    car->y = 0;

    trackPointIndex = FindTrackSegment(car, car->trackPointIndex);
    if (trackPointIndex < 0) {
        trackPointIndex = startPointIndex;
    }
    car->trackPointIndex = trackPointIndex;
    car->bodyPitch = 0;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    ReadStableRaceSeries() * ANGLE_HALF_TURN -
                    TrackPoint(trackPointIndex)->angle) & ANGLE_MASK;
    car->bodyRoll = 0;
    car->bodyRollVelocity = 0;
    car->progressB = 0;
    car->progressA = 0;
    car->trackProgress = 0;
    car->speed = 0;
    car->acceleration = 0;
    car->worldVelocityZ = 0;
    car->reservedCC = 0;
    car->worldVelocityX = 0;
    car->reservedE0 = 0;
    car->reservedDC = 0;
    car->reservedD8 = 0;
    car->motionZ = 0;
    car->motionY = 0;
    car->motionX = 0;
    car->routeIndex = 0;
    car->reserved116 = 0;
    car->reserved110 = 0;
    car->yawRate = 0;
    car->routeMarkerActive = 0;
    car->slideInput.value = 0;
    car->baseBodyYaw = car->bodyYaw;
    car->targetYaw = car->bodyYaw;
    car->headingAngle = car->bodyYaw;
    car->reservedF8 = 0;
    car->avoidanceActive = 0;
    car->reservedC4 = 0;
    car->routeMarkerIndex = 0;
    SeedCarLapProgress(car, start->modelId);

    car->activeFlag = start->modelId;
    if (start->modelId != -1) {
        UpdateCarTrackState(car, car->trackPointIndex, &trackLimits);
        car->modelY = car->y;
        car->previousTrackProgress = car->trackProgress;
    }

    car->avoidanceStep = 0;
    car->initialLateralOffset = car->trackLateralOffset;
    car->avoidanceTargetOffset = car->trackLateralOffset;
    car->aiLateralOffset = car->trackLateralOffset;
    CopyCarBodyRotationToModel(car);
    car->reserved40 = 0;
    car->steeringAngle = 0;
    car->wheelRotation = 0;
    car->modelY = car->y;
}
