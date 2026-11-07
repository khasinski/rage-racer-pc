#ifndef GAME_CAR_TRACK_INTERNAL_H
#define GAME_CAR_TRACK_INTERNAL_H

#include "game/car.h"
#include "game/integer.h"
#include "game/angle.h"
#include "game/track.h"
#include "psyq/gte.h"

s32 CalculateRouteOffsetHeading(const TrackRoute *route, s32 pointIndex,
                                 s32 segmentFraction, s32 carX, s32 carZ,
                                 s32 lateralOffset);
void SteerCarOnRoute(PlayerCarRuntime *car, const GameCarSpec *spec,
                      const TrackRoute *route);

void SeedCarTrackProgress(GameCarRuntime *car, const TrackRoute *route,
                          s32 startIndex, s32 seedSelector, int reverse);
void MoveCarTrackProgress(GameCarRuntime *car, const TrackRoute *route,
                          s32 target, int reverse);

void MeasureCarTrackLimits(const Matrix *toTrack,
                            const CarHullPoint corners[CAR_HULL_CORNER_COUNT],
                            CarTrackLimits *limits);
s32 ResolveCarTrackContact(PlayerCarRuntime *car, const TrackRoute *route,
                            const CarHullPoint corners[CAR_HULL_CORNER_COUNT],
                            int reverse);

s32 FindCarTrackSegment(const GameCarRuntime *car, const TrackRoute *route,
                         s32 startIndex);

s32 StepCarTrackState(GameCarRuntime *car, const TrackRoute *route,
                      s32 pointIndex, const CarTrackLimits *limits,
                      int reverse, int knockback);

typedef union CarTrackRadius {
    s32 value;
    struct {
        u16 low;
        u16 high;
    } half;
} CarTrackRadius;

typedef struct CarTrackWork {
    s32 arcCenterX;
    s32 arcCenterZ;
    s32 carToCenterX;
    s32 carToCenterZ;
    CarTrackRadius carRadius;
    CarTrackRadius pointRadius;
    CarTrackRadius nextPointRadius;
    s32 pointToCenterX;
    s32 nextPointToCenterX;
    s32 pointToCenterZ;
    s32 nextPointToCenterZ;
    s32 headingSin;
    s32 headingCos;
    s32 trackContact;
    SVec edgeOffset;
    LVec edgeCorrection;
    s16 curveMode;
    s16 arcIndex;
    s16 arcSpan;
    s16 sweptAngle;
    s16 pointAngle;
    s16 nextPointAngle;
    s16 arcLateral;
    s16 trackWidth;
    s16 rightHalfWidth;
    s16 leftHalfWidth;
    s16 relativeHeading;
    s16 crossSlope;
    s16 heading;
    s16 surfacePitch;
    s16 camberAngle;
    u16 segmentLength;
} CarTrackWork;

enum {
    CAR_TRACK_WORLD_COORDINATE_SCALE = 4,
    CAR_TRACK_SURFACE_HEIGHT_SHIFT = 7,
};

s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment,
                             s16 segmentLength);
s32 CarTrackFixed12ToInteger(s32 value);
s32 ProjectCarTrackAxis(s32 value);
s16 InterpolateCarTrackHeading(s16 pointHeading, s16 nextHeading,
                               s32 swept, s16 arcSpan);
/* Measure the car and a segment's endpoints relative to an arc centre. */
void CarTrackMeasureArc(CarTrackWork *work, const GameTrackArcCenter *arcCenter, s32 carX,
                        s32 carZ, const GameTrackPoint *point,
                        const GameTrackPoint *nextPoint);

static inline s32 CarRaceProgress(const GameCarRuntime *car) {
    return WrapSigned32((int64_t)car->progressA + car->progressB);
}

static inline s32 ClampCarTrackAlongSegment(s32 alongSegment,
                                            s16 segmentLength) {
    if (alongSegment < 0) {
        return 0;
    }
    if (alongSegment > segmentLength) {
        return segmentLength;
    }
    return alongSegment;
}

static inline void UpdateCarLapProgressState(GameCarRuntime *car,
                                             s32 trackLength, int reverse) {
    s32 progress;
    if (trackLength <= 0) {
        return;
    }
    progress = CarRaceProgress(car) % trackLength;
    s32 sectionProgress;

    car->previousTrackProgress = car->trackProgress;
    car->trackProgress = progress < 0 ? progress + trackLength : progress;
    sectionProgress = reverse
        ? trackLength - car->trackProgress
        : car->trackProgress;
    car->trackSection = WrapSigned16(sectionProgress >> 8);
}

static inline void MeasureCarTrackAxes(const GameCarRuntime *car,
                                       const GameTrackPoint *point,
                                       s32 heading, SVec *offset,
                                       s32 *alongSegment,
                                       s32 *lateralOffset) {
    s32 headingSin;
    s32 headingCos;

    offset->vx = WrapSigned16(
        ((u16)car->x - (u16)point->x) *
        CAR_TRACK_WORLD_COORDINATE_SCALE);
    offset->vy = 0;
    offset->vz = WrapSigned16(
        ((u16)car->z - (u16)point->z) *
        CAR_TRACK_WORLD_COORDINATE_SCALE);
    headingSin = SinAngle(heading);
    headingCos = CosAngle(heading);
    *alongSegment = ProjectCarTrackAxis(
        headingCos * offset->vx + headingSin * offset->vz);
    if (lateralOffset != NULL) {
        *lateralOffset = ProjectCarTrackAxis(
            -headingSin * offset->vx + headingCos * offset->vz);
    }
}

s32 CarFacesBackwards(const PlayerCarRuntime *car, const TrackRoute *route);

#endif
