#include "game/track_internal.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

void InitRivalCar(GameCarRuntime *ent, s32 pos, RaceGridSlot *slots) {
    const TrackRivalStart *start =
        &g_TrackEventData->rivalStarts[g_RaceSeries][pos + 1];
    CarTrackLimits trackLimits = {
        .rightInset = 20,
        .leftInset = -20,
    };
    s32 trackPointIndex;

    ent->initializedFlag = 1;
    ent->collisionFlag = 0;
    ent->aiEnabled = 1;
    ent->modelIndex = slots[pos].halves.modelId;
    ent->rivalModelId = slots[pos].halves.modelId;
    ent->trackPointIndex = start->trackPointIndex;
    ent->x = start->x;
    ent->z = start->z;
    ent->y = 0;

    trackPointIndex = FindTrackSegment(ent, ent->trackPointIndex);
    ent->trackPointIndex = trackPointIndex;
    ent->bodyPitch = 0;
    ent->bodyYaw =
        (0xC00 - (g_RaceSeries << 11) - TrackPoint(trackPointIndex)->angle) &
        0xFFF;
    ent->bodyRoll = 0;
    ent->bodyRollVelocity = 0;
    ent->progressB = 0;
    ent->progressA = 0;
    ent->trackProgress = 0;
    ent->speed = 0;
    ent->acceleration = 0;
    ent->worldVelocityZ = 0;
    ent->reservedCC = 0;
    ent->worldVelocityX = 0;
    ent->reservedE0 = 0;
    ent->reservedDC = 0;
    ent->reservedD8 = 0;
    ent->motionZ = 0;
    ent->motionY = 0;
    ent->motionX = 0;
    ent->routeIndex = 0;
    ent->reserved116 = 0;
    ent->reserved110 = 0;
    ent->yawRate = 0;
    ent->routeMarkerActive = 0;
    ent->slideInput.value = 0;
    ent->baseBodyYaw = ent->bodyYaw;
    ent->targetYaw = ent->bodyYaw;
    ent->headingAngle = ent->bodyYaw;
    ent->reservedF8 = 0;
    ent->avoidanceActive = 0;
    ent->reservedC4 = 0;
    ent->routeMarkerIndex = 0;
    SeedCarLapProgress(ent, start->modelId);

    ent->activeFlag = start->modelId;
    if (start->modelId != -1) {
        UpdateCarTrackState(ent, ent->trackPointIndex, &trackLimits);
        ent->modelY = ent->y;
        ent->previousTrackProgress = ent->trackProgress;
    }

    ent->avoidanceStep = 0;
    ent->initialLateralOffset = ent->trackLateralOffset;
    ent->avoidanceTargetOffset = ent->trackLateralOffset;
    ent->aiLateralOffset = ent->trackLateralOffset;
    CopyCarBodyRotationToModel(ent);
    ent->reserved40 = 0;
    ent->steeringAngle = 0;
    ent->wheelRotation = 0;
    ent->modelY = ent->y;
}
