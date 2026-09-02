#include <stdio.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track_internal.h"

void InstallTrackEventData(TrackEventData *eventData) {
    TrackEventOffsets *offsets = &eventData->offsets;
    u8 *base = (u8 *)offsets;

    g_TrackEventData = eventData;
    g_FlybySceneryData =
        (SceneryMotionData *)(base + offsets->flybyScenery);
    g_RaceIntroCameraScript =
        (RaceIntroCameraScript *)(base + offsets->raceIntroCamera);
    g_RouteSceneryData =
        (SceneryMotionData *)(base + offsets->routeScenery);
    g_PathSceneryPosData =
        (PathSceneryPositionData *)(base + offsets->pathSceneryPosition);
    g_PathSceneryRotData =
        (PathSceneryRotationData *)(base + offsets->pathSceneryRotation);
    printf("%s", g_MsgEventOk);
}
