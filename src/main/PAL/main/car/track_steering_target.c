#include "game/car_track_internal.h"

s32 CalculateTrackOffsetHeading(s32 pointIndex, s32 segmentFraction,
                                s32 carX, s32 carZ, s32 lateralOffset) {
    const TrackRoute route = {.points = g_TrackPoints, .count = g_TrackPointCount};
    return CalculateRouteOffsetHeading(&route, pointIndex, segmentFraction,
                                       carX, carZ, lateralOffset);
}
