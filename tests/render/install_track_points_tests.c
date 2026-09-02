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

    if (!InstallTrackPoints((TrackPointTable *)&fixture, sizeof(fixture))) {
        puts("FAIL: valid point table rejected");
        return 1;
    }
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

    fixture.count = 0;
    InstallTrackPoints((TrackPointTable *)&fixture, sizeof(fixture));
    if (g_TrackPoints != NULL || g_TrackPointCount != 0 ||
        g_TrackArcCenters != NULL || g_TrackLength != 0 ||
        g_TrackSectionCount != 0) {
        puts("FAIL: empty table was published");
        return 1;
    }

    g_TrackPoints = fixture.points;
    g_TrackPointCount = 3;
    g_TrackArcCenters = fixture.arcCenters;
    g_TrackLength = 600;
    g_TrackSectionCount = 3;
    InstallTrackPoints(NULL, 0);
    if (g_TrackPoints != NULL || g_TrackPointCount != 0 ||
        g_TrackArcCenters != NULL || g_TrackLength != 0 ||
        g_TrackSectionCount != 0) {
        puts("FAIL: null table was published");
        return 1;
    }

    fixture.count = 3;
    if (InstallTrackPoints(
            (TrackPointTable *)&fixture,
            offsetof(TrackFixture, points[2]) + sizeof(GameTrackPoint) - 1) !=
            0 ||
        g_TrackPoints != NULL || g_TrackPointCount != 0) {
        puts("FAIL: truncated point table was published");
        return 1;
    }

    puts("track point installation preserved");
    return 0;
}
