#include <stddef.h>

#include "game/track_internal.h"

/* Install the variable-length point table and its trailing arc-centre table. */
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
