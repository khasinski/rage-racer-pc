#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <string.h>

void InitRivalCar(GameCarRuntime *car,
                  s32 gridPosition,
                  const RaceGridSlot *grid) {
    const s32 series = g_RaceSeries != 0;
    const TrackRivalStart *start =
        &g_TrackEventData->rivalStarts[series][gridPosition + 1];
    const TrackRivalStart *position = start;
    CarTrackLimits trackLimits = {
        .rightInset = 20,
        .leftInset = -20,
    };
    s32 trackPointIndex;
    s32 startPointIndex;

    /* Use the last native grid position and its lap-progress seed without
     * changing the selected rival's model or AI configuration. */
    if (g_DuelEnabled && g_SceneId == GAME_SCENE_ENTER_RACE &&
        gridPosition == g_DuelRivalCar) {
        const TrackRivalStart *last =
            &g_TrackEventData->rivalStarts[series][RACE_CAR_SLOT_COUNT];
        if (last->activeFlag != -1) {
            position = last;
        }
    }

    memset(car, 0, sizeof(*car));
    car->initializedFlag = 1;
    car->aiEnabled = 1;
    car->facingBackwards = (s16)series;
    car->modelIndex = RaceGridModelId(grid[gridPosition]);
    car->rivalModelId = RaceGridModelId(grid[gridPosition]);
    startPointIndex = WrapTrackPointIndex(position->trackPointIndex);
    car->trackPointIndex = startPointIndex;
    car->x = position->x;
    car->z = position->z;

    trackPointIndex = FindTrackSegment(car, car->trackPointIndex);
    if (trackPointIndex < 0) {
        trackPointIndex = startPointIndex;
        car->x = TrackPoint(startPointIndex)->x;
        car->z = TrackPoint(startPointIndex)->z;
    }
    car->trackPointIndex = trackPointIndex;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    series * ANGLE_HALF_TURN -
                    TrackPoint(trackPointIndex)->angle) & ANGLE_MASK;
    car->baseBodyYaw = car->bodyYaw;
    car->targetYaw = car->bodyYaw;
    car->headingAngle = car->bodyYaw;
    SeedCarLapProgress(car, position->activeFlag);

    car->activeFlag = start->activeFlag;
    if (start->activeFlag != -1) {
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
