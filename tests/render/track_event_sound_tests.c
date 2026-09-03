#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

PlayerCarRuntime g_PlayerCar;
GameRenderState g_RenderState;
TrackEventData *g_TrackEventData;
GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
s32 g_MirrorMode;

static s32 g_LeftVolume;
static s32 g_RightVolume;

void SetPanVoiceTargetVolume(s32 left, s32 right) {
    g_LeftVolume = left;
    g_RightVolume = right;
}

static int ExpectVolumes(s16 section, s32 left, s32 right) {
    g_LeftVolume = -1;
    g_RightVolume = -1;
    UpdateTrackEventSound(section);
    if (g_LeftVolume != left || g_RightVolume != right) {
        printf("FAIL: section %d produced (%d, %d), expected (%d, %d)\n",
               section, g_LeftVolume, g_RightVolume, left, right);
        return 0;
    }
    return 1;
}

int main(void) {
    TrackEventData events;
    GameTrackPoint trackPoint;

    memset(&events, 0, sizeof(events));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&trackPoint, 0, sizeof(trackPoint));

    events.eventSoundZones[0] = (TrackEventSoundZone){10, 20, 1, 0};
    events.eventSoundZones[1] = (TrackEventSoundZone){30, 40, 2, 0};
    events.eventSoundZones[2].start = -1;
    g_TrackEventData = &events;
    g_TrackPoints = &trackPoint;
    g_TrackPointCount = 1;
    g_PlayerCar.trackPointIndex = 0;
    g_PlayerCar.speed = 12775;
    g_RenderState.viewAngleY = 0xC00;

    g_PlayerCar.normalizedLateralOffset = 0x200;
    if (!ExpectVolumes(10, 0, 0x200) ||
        !ExpectVolumes(20, 0, 0x200) ||
        !ExpectVolumes(21, 0, 0) ||
        !ExpectVolumes(30, 0, 0)) {
        return 1;
    }

    g_PlayerCar.normalizedLateralOffset = -0x200;
    if (!ExpectVolumes(30, 0x200, 0) ||
        !ExpectVolumes(40, 0x200, 0) ||
        !ExpectVolumes(41, 0, 0) ||
        !ExpectVolumes(10, 0, 0)) {
        return 1;
    }

    g_MirrorMode = 1;
    if (!ExpectVolumes(30, 0, 0x200)) {
        return 1;
    }

    g_PlayerCar.normalizedLateralOffset = 0x80;
    g_MirrorMode = 0;
    if (!ExpectVolumes(10, 0, 0)) {
        return 1;
    }

    g_TrackEventData = NULL;
    if (!ExpectVolumes(10, 0, 0)) {
        return 1;
    }
    g_TrackEventData = &events;
    g_TrackPoints = NULL;
    g_PlayerCar.normalizedLateralOffset = 0x200;
    if (!ExpectVolumes(10, 0, 0)) {
        return 1;
    }
    g_TrackPoints = &trackPoint;
    g_TrackPointCount = 0;
    if (!ExpectVolumes(10, 0, 0)) {
        return 1;
    }

    g_TrackPointCount = 1;
    g_PlayerCar.normalizedLateralOffset = INT_MIN;
    g_PlayerCar.speed = INT_MAX;
    g_RenderState.viewAngleY = INT_MIN;
    if (!ExpectVolumes(30, 990279935, 990279935)) {
        return 1;
    }

    puts("track event sound behavior preserved");
    return 0;
}
