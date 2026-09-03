#include "common.h"
#include "game/asset.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
const GameTrackArcCenter *g_TrackArcCenters;
s32 g_TrackLength;
u16 g_TrackSectionCount;

typedef struct TrackFixture {
    s32 count;
    GameTrackPoint points[3];
    GameTrackArcCenter arcCenters[2];
} TrackFixture;

int main(void) {
    TrackFixture fixture;
    u8 originalFixture[sizeof(fixture)];
    TrackPointTable *longTrack;
    size_t longTrackSize;
    s32 i;

    memset(&fixture, 0, sizeof(fixture));
    fixture.count = 3;
    fixture.points[0].segmentLength = 100;
    fixture.points[0].arcRef = (1 << 4) | TRACK_CURVE_PRIMARY;
    fixture.points[1].segmentLength = 200;
    fixture.points[2].segmentLength = 300;
    memcpy(originalFixture, &fixture, sizeof(fixture));

    if (TrackPointTableArcCenters(
            (const TrackPointTable *)(const void *)&fixture) !=
        fixture.arcCenters) {
        puts("FAIL: arc table asset layout");
        return 1;
    }

    if (!InstallTrackPoints(
            (const TrackPointTable *)(const void *)&fixture,
            sizeof(fixture))) {
        puts("FAIL: valid point table rejected");
        return 1;
    }
    if (memcmp(originalFixture, &fixture, sizeof(fixture)) != 0) {
        puts("FAIL: track point asset modified during installation");
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
            offsetof(TrackFixture, arcCenters[1])) != 0 ||
        g_TrackPoints != NULL || g_TrackArcCenters != NULL) {
        puts("FAIL: truncated arc-centre table was published");
        return 1;
    }
    fixture.points[0].arcRef = (u16)-15;
    if (InstallTrackPoints((TrackPointTable *)&fixture, sizeof(fixture)) != 0 ||
        g_TrackPoints != NULL || g_TrackArcCenters != NULL) {
        puts("FAIL: negative arc-centre index was published");
        return 1;
    }
    fixture.points[0].arcRef = (1 << 4) | TRACK_CURVE_PRIMARY;
    if (InstallTrackPoints(
            (TrackPointTable *)&fixture,
            offsetof(TrackFixture, points[2]) + sizeof(GameTrackPoint) - 1) !=
            0 ||
        g_TrackPoints != NULL || g_TrackPointCount != 0) {
        puts("FAIL: truncated point table was published");
        return 1;
    }

    fixture.points[0].segmentLength = 0;
    if (InstallTrackPoints((TrackPointTable *)&fixture, sizeof(fixture)) != 0 ||
        g_TrackPoints != NULL) {
        puts("FAIL: zero-length track segment was published");
        return 1;
    }
    fixture.points[0].segmentLength = 0x8000;
    if (InstallTrackPoints((TrackPointTable *)&fixture, sizeof(fixture)) != 0 ||
        g_TrackPoints != NULL) {
        puts("FAIL: negative signed track segment was published");
        return 1;
    }
    fixture.points[0].segmentLength = 100;
    fixture.points[0].arcRef = 3;
    if (InstallTrackPoints((TrackPointTable *)&fixture, sizeof(fixture)) != 0 ||
        g_TrackPoints != NULL) {
        puts("FAIL: unknown track curve mode was published");
        return 1;
    }
    fixture.points[0].arcRef = (1 << 4) | TRACK_CURVE_PRIMARY;

    longTrackSize = offsetof(TrackPointTable, points) +
                    256 * sizeof(GameTrackPoint);
    longTrack = calloc(1, longTrackSize);
    if (longTrack == NULL) {
        puts("FAIL: long track fixture allocation");
        return 1;
    }
    longTrack->count = 256;
    for (i = 0; i < longTrack->count - 1; i++) {
        longTrack->points[i].segmentLength = INT16_MAX;
    }
    longTrack->points[longTrack->count - 1].segmentLength = INT16_MAX - 1;
    if (InstallTrackPoints(longTrack, longTrackSize) == 0 ||
        g_TrackSectionCount != INT16_MAX) {
        free(longTrack);
        puts("FAIL: largest signed track section count was rejected");
        return 1;
    }
    longTrack->points[longTrack->count - 1].segmentLength = INT16_MAX;
    if (InstallTrackPoints(longTrack, longTrackSize) != 0 ||
        g_TrackPoints != NULL || g_TrackSectionCount != 0) {
        free(longTrack);
        puts("FAIL: track length overflowing section count was published");
        return 1;
    }
    free(longTrack);

    puts("track point installation preserved");
    return 0;
}
