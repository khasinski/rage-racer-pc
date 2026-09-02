#include <stdio.h>
#include "game/asset.h"
#include "game/track_internal.h"
#include "game/car.h"
#include "game/race.h"

void InstallResourceData(void *data) {
    (void)data;
    printf("%s", g_MsgResOk);
}

void SetCarSpec(GameCarSpec *spec) {
    g_CarSpec = spec;
}

void InstallTrackEventData(TrackEventData *eventData) {
    TrackEventOffsets *offsets = &eventData->offsets;
    u8 *base = (u8 *)offsets;

    g_TrackEventData = eventData;
    g_FlybySceneryData = (SceneryMotionData *)(base + offsets->flybyScenery);
    g_RaceIntroCameraScript = (RaceIntroCameraScript *)(base + offsets->raceIntroCamera);
    g_RouteSceneryData = (SceneryMotionData *)(base + offsets->routeScenery);
    g_PathSceneryPosData = (PathSceneryPositionData *)(base + offsets->pathSceneryPosition);
    g_PathSceneryRotData = (PathSceneryRotationData *)(base + offsets->pathSceneryRotation);
    printf("%s", g_MsgEventOk);
}
