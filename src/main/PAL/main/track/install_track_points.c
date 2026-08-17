#include "common.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"

/*
 * Installs the track-point table from a loaded blob: word 0 is the point count,
 * the rest is the GameTrackPoint array. Publishes g_TrackPoints (points),
 * g_TrackPointCount (count) and g_TrackArcCenters (marker array right after the points), then
 * sums every point's segmentLength into the total track length g_TrackLength and
 * derives g_TrackSectionCount = (total >> 8) + 1.
 */
void InstallTrackPoints(TrackPointTable *trackData) {
    s32 count;
    s32 limit;
    GameTrackPoint *points;
    s32 i;
    s32 index;
    GameTrackPoint *point;
    TrackPointTableAddress arcCenterAddress;
    TrackPointTableAddress pointAddress;
    s32 total;

    count = trackData->count;
    points = trackData->points;
    g_TrackPoints = points;
    g_TrackLength = 0;
    g_TrackPointCount = count;
    arcCenterAddress.pointPointer = points;
    arcCenterAddress.value =
        count * sizeof(GameTrackPoint) + arcCenterAddress.value;
    g_TrackArcCenters = arcCenterAddress.arcCenterPointer;

    i = 0;
    if (count > 0) {
        limit = count;
        do {
            index = i % limit;
            pointAddress.pointPointer = points;
            pointAddress.value =
                index * sizeof(GameTrackPoint) + pointAddress.value;
            point = pointAddress.pointPointer;
            g_TrackLength += (s16)point->segmentLength;
            i++;
        } while (i < limit);
    }

    total = g_TrackLength;
    g_TrackSectionCount = (total >> 8) + 1;
}
