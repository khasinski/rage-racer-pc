#include "common.h"
#include <stdio.h>
#include "game/asset.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/car.h"
#include "game/race.h"

#ifdef __psyz
#include "rage/automatic_transmission.h"
#endif


void InstallResourceData(void *data) {
    (void)data;
    printf("%s", g_MsgResOk);
}
void SetCarSpec(GameCarSpec *spec) {
#ifdef __psyz
    int automaticSelected = g_CarTable != NULL && g_PlayerCarIndex >= 0 &&
        g_PlayerCarIndex < 13 &&
        g_CarTable[g_PlayerCarIndex].transmission == 0;
    spec = RageAutomaticTransmissionSpec(spec, automaticSelected,
                                         g_CarModelAsset);
#endif
    g_CarSpec = spec;
}

void InstallTrackEventData(void *resourceData) {
    register AssetAddress cursor asm("$2");
    s32 offset1;
    u8 *callArg;
    TrackEventOffsetBase base;
    AssetAddress cameraAddress;
    TrackEventData *eventData;
    volatile TrackEventOffsets *offsets;

    eventData = resourceData;
    offsets = &eventData->offsets;
    cursor.offset = offsets->flybyScenery;
    offset1 = offsets->raceIntroCamera;
    base.offsets = offsets;
    g_TrackEventData = eventData;
    cursor.pointer = base.bytes + cursor.offset;
    g_FlybySceneryData = cursor.sceneryMotion;
    cursor.offset = offsets->routeScenery;
    cameraAddress.pointer = base.bytes + offset1;
    g_RaceIntroCameraScript = cameraAddress.raceIntroCamera;
    offset1 = offsets->pathSceneryRotation;
    cursor.pointer = base.bytes + cursor.offset;
    g_RouteSceneryData = cursor.sceneryMotion;
    cursor.offset = offsets->pathSceneryPosition;
    callArg = g_MsgEventOk;
    cursor.pointer = base.bytes + cursor.offset;
    base.bytes += offset1;
    g_PathSceneryPosData = cursor.pathSceneryPosition;
    g_PathSceneryRotData = base.pathSceneryRotation;
    printf("%s", callArg);
}
