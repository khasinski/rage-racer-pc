#include "game/angle.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <string.h>

void InitRivalCar(GameCarRuntime *car,
                  s32 gridPosition,
                  const RaceGridSlot *grid) {
    const TrackRivalStart *start =
        &g_TrackEventData->rivalStarts[g_RaceSeries][gridPosition + 1];
    CarTrackLimits trackLimits = {
        .rightInset = 20,
        .leftInset = -20,
    };
    s32 trackPointIndex;
    s32 startPointIndex;

    memset(car, 0, sizeof(*car));
    car->initializedFlag = 1;
    car->aiEnabled = 1;
    car->facingBackwards = (s16)g_RaceSeries;
    car->modelIndex = grid[gridPosition].halves.modelId;
    car->rivalModelId = grid[gridPosition].halves.modelId;
    startPointIndex = WrapTrackPointIndex(start->trackPointIndex);
    car->trackPointIndex = startPointIndex;
    car->x = start->x;
    car->z = start->z;

    trackPointIndex = FindTrackSegment(car, car->trackPointIndex);
    if (trackPointIndex < 0) {
        trackPointIndex = startPointIndex;
    }
    car->trackPointIndex = trackPointIndex;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    g_RaceSeries * ANGLE_HALF_TURN -
                    TrackPoint(trackPointIndex)->angle) & ANGLE_MASK;
    car->baseBodyYaw = car->bodyYaw;
    car->targetYaw = car->bodyYaw;
    car->headingAngle = car->bodyYaw;
    SeedCarLapProgress(car, start->modelId);

    car->activeFlag = start->modelId;
    if (start->modelId != -1) {
        UpdateCarTrackState(car, car->trackPointIndex, &trackLimits);
        car->modelY = car->y;
        car->previousTrackProgress = car->trackProgress;
    }

    car->initialLateralOffset = car->trackLateralOffset;
    car->avoidanceTargetOffset = car->trackLateralOffset;
    car->aiLateralOffset = car->trackLateralOffset;
    CopyCarBodyRotationToModel(car);
    car->modelY = car->y;
}
