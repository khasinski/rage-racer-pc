#include "game/track.h"
#include <stddef.h>

enum { TRACK_LENGTH_MAX = ((u32)INT16_MAX << 8) - 1 };

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

int ReadTrackRoute(const TrackPointTable *table, size_t size, TrackRoute *route) {
    if (route == NULL) return 0;
    *route = (TrackRoute){0};
    if (!IsValidTrackPointAsset(table, size)) return 0;
    s32 length = 0;
    for (s32 i = 0; i < table->count; i++) length += (s16)table->points[i].segmentLength;
    *route = (TrackRoute){.points = table->points,
        .arcs = TrackPointTableArcCenters(table), .count = table->count, .length = length};
    return 1;
}
