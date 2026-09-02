#include <stddef.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/track_internal.h"

static void *ResolveTrackEventOffset(TrackEventOffsets *offsets, s32 offset) {
    if (offset < (s32)sizeof(*offsets) || offset % (s32)sizeof(s32) != 0) {
        return NULL;
    }

    return (u8 *)offsets + offset;
}

void InstallTrackEventData(TrackEventData *eventData) {
    TrackEventOffsets *offsets;

    if (eventData == NULL) {
        g_TrackEventData = NULL;
        g_FlybySceneryData = NULL;
        g_RaceIntroCameraScript = NULL;
        g_RouteSceneryData = NULL;
        g_PathSceneryPosData = NULL;
        g_PathSceneryRotData = NULL;
        return;
    }

    offsets = &eventData->offsets;
    g_TrackEventData = eventData;
    g_FlybySceneryData =
        ResolveTrackEventOffset(offsets, offsets->flybyScenery);
    g_RaceIntroCameraScript =
        ResolveTrackEventOffset(offsets, offsets->raceIntroCamera);
    g_RouteSceneryData =
        ResolveTrackEventOffset(offsets, offsets->routeScenery);
    g_PathSceneryPosData =
        ResolveTrackEventOffset(offsets, offsets->pathSceneryPosition);
    g_PathSceneryRotData =
        ResolveTrackEventOffset(offsets, offsets->pathSceneryRotation);
}
