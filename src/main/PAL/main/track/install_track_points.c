#include <stddef.h>

#include "game/track_internal.h"

enum {
    /* Track positions and camera sections are stored as signed halfwords. */
    TRACK_LENGTH_MAX = ((u32)INT16_MAX << 8) - 1,
};

static void ClearTrackPoints(void) {
    g_TrackPoints = NULL;
    g_TrackPointCount = 0;
    g_TrackArcCenters = NULL;
    g_TrackLength = 0;
    g_TrackSectionCount = 0;
}

s32 IsValidTrackPointAsset(const TrackPointTable *trackData, size_t size) {
    size_t pointBytes;
    size_t arcCenterCount = 0;
    u32 trackLength = 0;
    s32 i;

    if (trackData == NULL || size < offsetof(TrackPointTable, points)) {
        return 0;
    }
    if (trackData->count <= 0 ||
        (size_t)trackData->count >
            (size - offsetof(TrackPointTable, points)) /
                sizeof(trackData->points[0])) {
        return 0;
    }
    pointBytes = offsetof(TrackPointTable, points) +
                 (size_t)trackData->count * sizeof(trackData->points[0]);
    for (i = 0; i < trackData->count; i++) {
        const GameTrackPoint *point = &trackData->points[i];
        TrackCurveMode curveMode = TrackPointCurveMode(point);
        s32 segmentLength = (s16)point->segmentLength;

        if (segmentLength <= 0 ||
            trackLength > TRACK_LENGTH_MAX - (u32)segmentLength ||
            curveMode > TRACK_CURVE_MIRRORED) {
            return 0;
        }
        trackLength += (u32)segmentLength;
        if (curveMode != TRACK_CURVE_NONE) {
            s32 arcIndex = TrackPointArcIndex(point);

            if (arcIndex < 0) return 0;
            if ((size_t)arcIndex >= arcCenterCount) {
                arcCenterCount = (size_t)arcIndex + 1;
            }
        }
    }
    return arcCenterCount <=
           (size - pointBytes) / sizeof(GameTrackArcCenter);
}

/* Install the variable-length point table and its trailing arc-centre table. */
s32 InstallTrackPoints(TrackPointTable *trackData, size_t size) {
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
