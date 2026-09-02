#include <stddef.h>
#include <stdio.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track_internal.h"

typedef union TrackEventAssetAddress {
    TrackEventOffsets *offsets;
    u8 *bytes;
    void *pointer;
} TrackEventAssetAddress;

static void *ResolveTrackEventOffset(TrackEventOffsets *offsets, s32 offset) {
    TrackEventAssetAddress address;

    if (offset < (s32)sizeof(*offsets) || offset % (s32)sizeof(s32) != 0) {
        return NULL;
    }

    address.offsets = offsets;
    address.bytes += offset;
    return address.pointer;
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
    printf("%s", g_MsgEventOk);
}
