#include <stddef.h>

#include "game/track_internal.h"

static void ClearTrackPoints(void) {
    g_TrackPoints = NULL;
    g_TrackPointCount = 0;
    g_TrackArcCenters = NULL;
    g_TrackLength = 0;
    g_TrackSectionCount = 0;
}

/* Install the variable-length point table and its trailing arc-centre table. */
s32 InstallTrackPoints(TrackPointTable *trackData, size_t size) {
    s32 i;

    if (trackData == NULL || size < offsetof(TrackPointTable, points)) {
        ClearTrackPoints();
        return 0;
    }
    if (trackData->count <= 0 ||
        (size_t)trackData->count >
            (size - offsetof(TrackPointTable, points)) /
                sizeof(trackData->points[0])) {
        ClearTrackPoints();
        return 0;
    }
    g_TrackPoints = trackData->points;
    g_TrackLength = 0;
    g_TrackPointCount = trackData->count;
    g_TrackArcCenters = TrackPointTableArcCenters(trackData);

    for (i = 0; i < trackData->count; i++) {
        g_TrackLength += (s16)trackData->points[i].segmentLength;
    }

    g_TrackSectionCount = (g_TrackLength >> 8) + 1;
    return 1;
}
