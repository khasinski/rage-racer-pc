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
s32 InstallTrackPoints(const TrackPointTable *trackData, size_t size) {
    TrackRoute route;
    if (!ReadTrackRoute(trackData, size, &route)) {
        ClearTrackPoints();
        return 0;
    }
    g_TrackPoints = route.points;
    g_TrackLength = route.length;
    g_TrackPointCount = route.count;
    g_TrackArcCenters = route.arcs;
    g_TrackSectionCount = (route.length >> 8) + 1;
    return 1;
}
