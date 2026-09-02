#include "common.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

TrackEventData *g_TrackEventData;
GameRenderState g_RenderState;
s32 g_RaceSeries;
s32 g_TrackLength;
s32 g_MirrorMode;

static s32 g_Cue;
static s32 g_LeftVolume;
static s32 g_RightVolume;
static s32 g_Sine = 4096;

long SquareRoot12(long value) {
    (void)value;
    return 0;
}

s32 Atan2(s32 x, s32 y) {
    (void)x;
    (void)y;
    return 0;
}

s32 rsin(s32 angle) {
    (void)angle;
    return g_Sine;
}

void SetStereoSoundCue(s32 cue, s32 left, s32 right) {
    g_Cue = cue;
    g_LeftVolume = left;
    g_RightVolume = right;
}

static int ExpectAmbience(s32 position, s32 cue, s32 left, s32 right) {
    g_Cue = -1;
    g_LeftVolume = -1;
    g_RightVolume = -1;
    UpdatePointAmbience(position);
    if (g_Cue != cue || g_LeftVolume != left || g_RightVolume != right) {
        printf("FAIL: position %d produced cue %d (%d, %d), expected %d (%d, %d)\n",
               position, g_Cue, g_LeftVolume, g_RightVolume,
               cue, left, right);
        return 0;
    }
    return 1;
}

int main(void) {
    TrackEventData events;

    memset(&events, 0, sizeof(events));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    events.pointAmbienceZones[0] =
        (TrackPointAmbienceZone){100, 200, 20, 20, 10, 20, -1};
    events.pointAmbienceZones[1] =
        (TrackPointAmbienceZone){300, 400, 20, 20, 30, 40, 2};
    g_TrackEventData = &events;
    g_TrackLength = 1000;

    if (!ExpectAmbience(50, 3, 0, 0) ||
        !ExpectAmbience(100, 3, 0, 0) ||
        !ExpectAmbience(110, 2, 32, 80) ||
        !ExpectAmbience(150, 2, 32, 128) ||
        !ExpectAmbience(390, 3, 32, 80)) {
        return 1;
    }

    events.pointAmbienceZones[0].fadeInDistance = 0;
    events.pointAmbienceZones[0].fadeOutDistance = 0;
    if (!ExpectAmbience(150, 2, 32, 128)) {
        return 1;
    }
    events.pointAmbienceZones[0].fadeInDistance = 20;
    events.pointAmbienceZones[0].fadeOutDistance = 20;

    g_RaceSeries = 1;
    if (!ExpectAmbience(890, 2, 32, 80)) {
        return 1;
    }

    g_MirrorMode = 1;
    if (!ExpectAmbience(850, 2, 128, 32)) {
        return 1;
    }

    g_Sine = -4096;
    if (!ExpectAmbience(850, 2, 32, 128)) {
        return 1;
    }

    puts("point ambience behavior preserved");
    return 0;
}
