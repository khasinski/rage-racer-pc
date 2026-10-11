#include <stddef.h>

#include "game/track_internal.h"
#include "content/track_pack.h"

static void ClearTrackPoints(void) {
    g_TrackPoints = NULL;
    g_TrackPointCount = 0;
    g_TrackArcCenters = NULL;
    g_TrackLength = 0;
    g_TrackSectionCount = 0;
}

s32 IsValidTrackPointAsset(const TrackPointTable *trackData, size_t size) {
    Road road;
    return RoadOpen(trackData, size, &road);
}

/* Install the variable-length point table and its trailing arc-centre table. */
s32 InstallTrackPoints(const TrackPointTable *trackData, size_t size) {
    s32 i;

    if (!IsValidTrackPointAsset(trackData, size)) {
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
