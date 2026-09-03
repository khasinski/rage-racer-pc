#ifndef GAME_CAR_TRACK_INTERNAL_H
#define GAME_CAR_TRACK_INTERNAL_H

#include "game/car.h"
#include "game/integer.h"
#include "game/render.h"
#include "game/track.h"

s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment,
                             s16 segmentLength);
s32 CarTrackFixed12ToInteger(s32 value);
s32 ProjectCarTrackAxis(s32 value);
s16 InterpolateCarTrackHeading(s16 pointHeading, s16 nextHeading,
                               s32 swept, s16 arcSpan);

static inline void MeasureCarTrackAxes(const GameCarRuntime *car,
                                       const GameTrackPoint *point,
                                       s32 heading, SVec *offset,
                                       s32 *alongSegment,
                                       s32 *lateralOffset) {
    s32 headingSin;
    s32 headingCos;

    offset->vx = WrapSigned16(((u16)car->x - (u16)point->x) * 4);
    offset->vy = 0;
    offset->vz = WrapSigned16(((u16)car->z - (u16)point->z) * 4);
    headingSin = rsin(heading);
    headingCos = rcos(heading);
    *alongSegment = ProjectCarTrackAxis(
        headingCos * offset->vx + headingSin * offset->vz);
    if (lateralOffset != NULL) {
        *lateralOffset = ProjectCarTrackAxis(
            -headingSin * offset->vx + headingCos * offset->vz);
    }
}

#endif
