#include "game/track_internal.h"

/*
 * Installs the track-point table from a loaded blob: word 0 is the point count,
 * the rest is the GameTrackPoint array. Publishes g_TrackPoints (points),
 * g_TrackPointCount (count) and g_TrackArcCenters (marker array right after the points), then
 * sums every point's segmentLength into the total track length g_TrackLength and
 * derives g_TrackSectionCount = (total >> 8) + 1.
 */
void InstallTrackPoints(TrackPointTable *trackData) {
    s32 count;
    GameTrackPoint *points;
    s32 i;

    count = trackData->count;
    points = trackData->points;
    g_TrackPoints = points;
    g_TrackLength = 0;
    g_TrackPointCount = count;
    g_TrackArcCenters = (GameTrackArcCenter *)(void *)&points[count];

    for (i = 0; i < count; i++)
        g_TrackLength += (s16)points[i].segmentLength;

    g_TrackSectionCount = (g_TrackLength >> 8) + 1;
}
