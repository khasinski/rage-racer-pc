#include "common.h"
#include "game/asset.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
GameTrackArcCenter *g_TrackArcCenters;
s32 g_TrackLength;
u16 g_TrackSectionCount;

typedef struct TrackFixture {
    s32 count;
    GameTrackPoint points[3];
    GameTrackArcCenter arcCenters[2];
} TrackFixture;

int main(void) {
    TrackFixture fixture;

    memset(&fixture, 0, sizeof(fixture));
    fixture.count = 3;
    fixture.points[0].segmentLength = 100;
    fixture.points[1].segmentLength = 200;
    fixture.points[2].segmentLength = 300;

    if (TrackPointTableArcCenters((TrackPointTable *)&fixture) !=
        fixture.arcCenters) {
        puts("FAIL: arc table asset layout");
        return 1;
    }

    InstallTrackPoints((TrackPointTable *)&fixture);
    if (g_TrackPointCount != 3 ||
        g_TrackPoints != fixture.points ||
        g_TrackArcCenters != fixture.arcCenters ||
        g_TrackLength != 600 ||
        g_TrackSectionCount != 3) {
        printf("FAIL: points=%p arcs=%p count=%d length=%d sections=%u\n",
               (void *)g_TrackPoints, (void *)g_TrackArcCenters,
               g_TrackPointCount, g_TrackLength, g_TrackSectionCount);
        return 1;
    }

    puts("track point installation preserved");
    return 0;
}
