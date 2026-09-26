#include <stddef.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/track_internal.h"

static void ClearTrackEventData(void) {
    g_TrackEventData = NULL;
    g_FlybySceneryData = NULL;
    g_RaceIntroCameraScript = NULL;
    g_RouteSceneryData = NULL;
    g_PathSceneryPosData = NULL;
    g_PathSceneryRotData = NULL;
}

s32 InstallTrackEventData(const TrackEventData *eventData, size_t size) {
    const TrackEventOffsets *offsets;

    eventData = ReadTrackEvents(eventData, size);
    if (eventData == NULL) {
        ClearTrackEventData();
        return 0;
    }
    offsets = &eventData->offsets;
    g_FlybySceneryData = (const SceneryMotionData *)(
        (const u8 *)offsets + offsets->flybyScenery);
    g_RaceIntroCameraScript = (const RaceIntroCameraScript *)(
        (const u8 *)offsets + offsets->raceIntroCamera);
    g_RouteSceneryData = (const SceneryMotionData *)(
        (const u8 *)offsets + offsets->routeScenery);
    g_PathSceneryPosData = (const PathSceneryPositionData *)(
        (const u8 *)offsets + offsets->pathSceneryPosition);
    g_PathSceneryRotData = (const PathSceneryRotationData *)(
        (const u8 *)offsets + offsets->pathSceneryRotation);
    g_TrackEventData = eventData;
    return 1;
}
