#include "common.h"
#include "game/race.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

const TrackEventData *g_TrackEventData;
s32 g_GrandPrixClass;
s32 g_RaceSeries;
s32 g_TrackLength;

static s32 g_Cue;
static s32 g_Left;
static s32 g_Right;

void SetStereoSoundCue(s32 cue, s32 left, s32 right) {
    g_Cue = cue;
    g_Left = left;
    g_Right = right;
}

static int Expect(s32 position, s32 cue, s32 volume) {
    UpdateZoneAmbience(position);
    if (g_Cue != cue || g_Left != volume || g_Right != volume) {
        printf("FAIL position %d: cue=%d volumes=(%d,%d), expected %d/%d\n",
               position, g_Cue, g_Left, g_Right, cue, volume);
        return 0;
    }
    return 1;
}

int main(void) {
    TrackEventData events;

    memset(&events, 0, sizeof(events));
    events.ambienceZones[0] = (TrackAmbienceZone){100, 2100, 0, 3};
    events.ambienceZones[1].start = -1;
    g_TrackEventData = &events;
    g_TrackLength = 3000;

    g_GrandPrixClass = 1;
    if (!Expect(50, 0, 0) ||
        !Expect(100, 0, 0) ||
        !Expect(500, 0, 48) ||
        !Expect(1000, 0, 96) ||
        !Expect(1700, 0, 48) ||
        !Expect(2100, 0, 0)) {
        return 1;
    }

    g_GrandPrixClass = 3;
    if (!Expect(1000, 1, 96)) return 1;

    g_GrandPrixClass = 5;
    if (!Expect(1000, 0, 0)) return 1;

    g_GrandPrixClass = 4;
    g_RaceSeries = 1;
    if (!Expect(2000, 1, 96)) return 1;

    g_TrackEventData = NULL;
    if (!Expect(2000, 1, 0)) return 1;

    g_TrackEventData = &events;
    g_TrackLength = INT_MIN;
    events.ambienceZones[0] = (TrackAmbienceZone){1, 1, 0, 0};
    if (!Expect(INT_MAX, 1, 96)) return 1;

    puts("zone ambience behavior preserved");
    return 0;
}
