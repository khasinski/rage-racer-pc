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

static void *ResolveTrackEventOffset(TrackEventOffsets *offsets, s32 offset,
                                     size_t remaining) {
    if (offset < (s32)sizeof(*offsets) ||
        offset % (s32)sizeof(s32) != 0 || (size_t)offset >= remaining) {
        return NULL;
    }

    return (u8 *)offsets + offset;
}

s32 InstallTrackEventData(TrackEventData *eventData, size_t size) {
    TrackEventOffsets *offsets;
    size_t offsetsPosition = offsetof(TrackEventData, offsets);
    size_t remaining;

    if (eventData == NULL || size < sizeof(*eventData)) {
        ClearTrackEventData();
        return 0;
    }

    offsets = &eventData->offsets;
    remaining = size - offsetsPosition;
    g_FlybySceneryData = ResolveTrackEventOffset(
        offsets, offsets->flybyScenery, remaining);
    g_RaceIntroCameraScript = ResolveTrackEventOffset(
        offsets, offsets->raceIntroCamera, remaining);
    g_RouteSceneryData = ResolveTrackEventOffset(
        offsets, offsets->routeScenery, remaining);
    g_PathSceneryPosData = ResolveTrackEventOffset(
        offsets, offsets->pathSceneryPosition, remaining);
    g_PathSceneryRotData = ResolveTrackEventOffset(
        offsets, offsets->pathSceneryRotation, remaining);
    if (g_FlybySceneryData == NULL || g_RaceIntroCameraScript == NULL ||
        g_RouteSceneryData == NULL || g_PathSceneryPosData == NULL ||
        g_PathSceneryRotData == NULL) {
        ClearTrackEventData();
        return 0;
    }
    g_TrackEventData = eventData;
    return 1;
}
