#include <stddef.h>

#include "game/track_internal.h"

/*
 * Installs the track-point table from a loaded blob: word 0 is the point count,
 * the rest is the GameTrackPoint array. Publishes g_TrackPoints (points),
 * g_TrackPointCount (count) and g_TrackArcCenters (marker array right after the points), then
 * sums every point's segmentLength into the total track length g_TrackLength and
 * derives g_TrackSectionCount = (total >> 8) + 1.
 */
void InstallTrackPoints(TrackPointTable *trackData) {
    s32 i;

    if (trackData == NULL || trackData->count <= 0) {
        g_TrackPoints = NULL;
        g_TrackPointCount = 0;
        g_TrackArcCenters = NULL;
        g_TrackLength = 0;
        g_TrackSectionCount = 0;
        return;
    }

    g_TrackPoints = trackData->points;
    g_TrackLength = 0;
    g_TrackPointCount = trackData->count;
    g_TrackArcCenters = TrackPointTableArcCenters(trackData);

    for (i = 0; i < trackData->count; i++) {
        g_TrackLength += (s16)trackData->points[i].segmentLength;
    }

    g_TrackSectionCount = (g_TrackLength >> 8) + 1;
}
