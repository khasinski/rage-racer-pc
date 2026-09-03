#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

TrackEventData *g_TrackEventData;
s32 g_RaceSeries;
s32 g_TrackLength;
s16 g_TrackZoneCode;
s16 g_ReverbZoneDepth;
s16 g_TrackZoneDark;

static int Expect(const char *label, s32 position, s32 blend,
                  s32 code, s32 depth, s32 dark) {
    s32 actual = GetTrackZoneBlend(position);

    if (actual != blend || g_TrackZoneCode != code ||
        g_ReverbZoneDepth != depth || g_TrackZoneDark != dark) {
        printf("FAIL %s: blend=%d code=%d depth=%d dark=%d\n",
               label, actual, g_TrackZoneCode,
               g_ReverbZoneDepth, g_TrackZoneDark);
        return 0;
    }
    return 1;
}

int main(void) {
    TrackEventData events;

    memset(&events, 0, sizeof(events));
    events.zones[0] = (TrackZone){100, 1000, 1, 7};
    events.zones[1].start = -1;
    g_TrackEventData = &events;
    g_TrackLength = 2000;

    if (!Expect("strict start", 100, 0, 0, 0, 0) ||
        !Expect("fade in", 101, 1, 1, 7, 0) ||
        !Expect("inside", 500, 0x100, 1, 7, 0) ||
        !Expect("fade out", 999, 1, 1, 7, 0) ||
        !Expect("strict end", 1000, 0, 0, 0, 0)) {
        return 1;
    }

    events.zones[0].code = 0;
    if (!Expect("dark-only zone", 500, 0x100, 0, 7, 3)) return 1;

    events.zones[0].code = 2;
    if (!Expect("code two", 500, 0, 1, 7, 0)) return 1;

    events.zones[0].code = -3;
    if (!Expect("code minus three entry", 101, 1, 1, 7, 0) ||
        !Expect("code minus three exit", 999, 0x100, 1, 7, 0)) {
        return 1;
    }

    events.zones[0].code = -4;
    if (!Expect("negative zone code", 101, 0x100, 4, 7, 0)) return 1;

    events.zones[0].code = 1;
    g_RaceSeries = 1;
    if (!Expect("reverse series", 1899, 1, 1, 7, 0)) return 1;

    g_TrackEventData = NULL;
    g_TrackZoneCode = 9;
    g_ReverbZoneDepth = 10;
    g_TrackZoneDark = 11;
    if (!Expect("no installed track events", 500, 0, 0, 0, 0)) return 1;

    puts("track zone blend behavior preserved");
    return 0;
}
