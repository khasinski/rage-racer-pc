#include "game/diagnostics.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Work out where the track's edges are here, and hold the car inside them.
 *
 * Both half-widths are interpolated along the segment and widened by their own
 * inset; a car outside either is pushed back to the edge and reported as
 * having hit it. Only the player's car is moved by the push, a rival is told
 * about it and left where it is.
 */
static s32 ClampCarToTrackEdges(GameCarRuntime *obj, CarTrackWork *work,
                                const CarTrackLimits *limits,
                                const GameTrackPoint *point,
                                const GameTrackPoint *nextPoint,
                                s32 alongSegment, s32 lateralOffset) {
    s32 edgeHeight;
    s32 leftLimit;
    s32 rightLimit;
    s16 segmentLength;

    segmentLength = (s16)work->segmentLength;
    work->leftHalfWidth = (s16)((nextPoint->leftHalfWidth * alongSegment +
                           point->leftHalfWidth * (segmentLength - alongSegment)) /
                          segmentLength);
    edgeHeight = (nextPoint->rightHalfWidth * alongSegment +
                  point->rightHalfWidth * (segmentLength - alongSegment)) /
                 segmentLength;
    work->rightHalfWidth = (s16)edgeHeight;
    leftLimit = work->leftHalfWidth + limits->leftInset;
    if (lateralOffset < -leftLimit) {
        lateralOffset += leftLimit;
        work->offsetX = 0U;
        work->offsetY = 0;
        work->offsetZ = lateralOffset;
        BuildRotMatrixY(&work->pad40[0], work->heading);
        ApplyMatrix(&work->pad40[0], &work->offsetX, &work->correctionX);
        if (obj == (GameCarRuntime *)&g_PlayerCar) {
            SetCarKnockback(obj, work->correctionX, work->correctionZ, limits->leftKnockbackMode);
        }
        obj->x -= work->correctionX;
        obj->z -= work->correctionZ;
        lateralOffset = -work->leftHalfWidth - limits->leftInset;
        work->knockbackMode = limits->leftKnockbackMode;
        return lateralOffset;
    }
    rightLimit = (s16)edgeHeight - limits->rightInset;
    if (rightLimit < lateralOffset) {
        lateralOffset -= rightLimit;
        work->offsetX = 0U;
        work->offsetY = 0;
        work->offsetZ = lateralOffset;
        BuildRotMatrixY(&work->pad40[0], work->heading);
        ApplyMatrix(&work->pad40[0], &work->offsetX, &work->correctionX);
        if (obj == (GameCarRuntime *)&g_PlayerCar) {
            SetCarKnockback(obj, work->correctionX, work->correctionZ, limits->rightKnockbackMode);
        }
        obj->x -= work->correctionX;
        obj->z -= work->correctionZ;
        lateralOffset = work->rightHalfWidth - limits->rightInset;
        work->knockbackMode = limits->rightKnockbackMode;
    }
    return lateralOffset;
}

/*
 * Place a car that is on a corner.
 *
 * A bend is described by the centre it turns about, so how far round the
 * segment the car has come is an angle rather than a distance: the angle from
 * the centre to the car, measured against the angles to the segment's two
 * ends. The difference between the car's radius and the centreline's is how
 * far off the line it is, and cornering model 2 is the mirrored hand, so its
 * offset comes out negated.
 *
 * Everything it works out is left in that struct, which is where the rest
 * of the placement reads it from.
 */
static void PlaceCarOnArc(GameCarRuntime *obj, CarTrackWork *work,
                          const GameTrackPoint *point,
                          const GameTrackPoint *nextPoint, s32 arcIndex) {
    s32 arcAngle;
    s32 arcLateral;
    s16 arcSpan;
    s32 headingAngle;
    s32 interpolatedRadius;
    s32 pointHeading;
    s32 swept;
    s32 sweptAngle;

    CarTrackMeasureArc(work, arcIndex, obj->x, obj->z, point, nextPoint);
    work->arcSpan = GetAngleDistance(work->pointAngle, work->nextPointAngle);
    sweptAngle = GetAngleDistance(work->pointAngle, work->sweptAngle);
    arcAngle = work->arcSpan;
    work->sweptAngle = sweptAngle;
    if (arcAngle <= 0) {
        interpolatedRadius = work->pointRadius.value;
        work->arcSpan = 1;
    } else {
        interpolatedRadius = (((s16)sweptAngle * work->pointRadius.value) +
                              ((arcAngle - (s16)sweptAngle) * work->nextPointRadius.value)) /
                             arcAngle;
    }
    work->pointRadius.value = interpolatedRadius;
    arcLateral = (s16)(work->carRadius.half.low - work->pointRadius.half.low);
    if (work->curveMode == 2) {
        arcLateral = -arcLateral;
    }
    work->arcLateral = arcLateral;
    headingAngle = nextPoint->angle;
    pointHeading = point->angle;
    if ((headingAngle - pointHeading) >= 0x801) {
        swept = work->sweptAngle;
        arcSpan = work->arcSpan;
        work->heading = (s16)(((headingAngle - 0x1000) * swept +
                               pointHeading * (arcSpan - swept)) /
                              arcSpan);
    } else if ((pointHeading - headingAngle) >= 0x801) {
        swept = work->sweptAngle;
        arcSpan = work->arcSpan;
        work->heading = (s16)((headingAngle * swept +
                               (pointHeading - 0x1000) * (arcSpan - swept)) /
                              arcSpan);
    } else {
        swept = work->sweptAngle;
        arcSpan = work->arcSpan;
        work->heading =
            (s16)((headingAngle * swept + pointHeading * (arcSpan - swept)) / arcSpan);
    }
}

static s32 InterpolateTrackValue(s32 start, s32 end, s32 alongSegment,
                                 s16 segmentLength) {
    return (end * alongSegment + start * (segmentLength - alongSegment)) /
           segmentLength;
}

/* The original fixed-point maths rounds negative products towards zero. */
static s32 ShiftFixed12(s32 value) {
    if (value < 0) {
        value += 0xFFF;
    }
    return value >> 12;
}

static s32 ProjectTrackAxis(s32 value) {
    if (value < 0) {
        value += 0xFFF;
    }
    return value >> 14;
}

static void UpdateCarSurfaceOrientation(GameCarRuntime *obj,
                                        CarTrackWork *work,
                                        const GameTrackPoint *point,
                                        const GameTrackPoint *nextPoint,
                                        s32 alongSegment,
                                        s32 lateralOffset) {
    s16 segmentLength = (s16)work->segmentLength;
    s16 trackWidth;
    s32 nextCamber;
    s32 pointCamber;
    s32 surfaceHeight;

    work->crossSlope = (s16)InterpolateTrackValue(
        point->crossSlope, nextPoint->crossSlope, alongSegment, segmentLength);
    surfaceHeight = InterpolateTrackValue(
        point->y, nextPoint->y, alongSegment, segmentLength);
    obj->y = ((work->crossSlope * lateralOffset) >> 7) + surfaceHeight;

    work->relativeHeading = (s16)((u16)obj->bodyYaw - 0xC00 +
                                  (u16)work->heading);
    work->surfacePitch = (s16)InterpolateTrackValue(
        point->surfacePitch, nextPoint->surfacePitch, alongSegment,
        segmentLength);

    trackWidth = (u16)work->rightHalfWidth + (u16)work->leftHalfWidth;
    work->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth,
                       (nextPoint->crossSlope * trackWidth) >> 7);
    pointCamber = Atan2(work->trackWidth,
                        (point->crossSlope * work->trackWidth) >> 7);
    work->camberAngle = (s16)InterpolateTrackValue(
        pointCamber, nextCamber, alongSegment, segmentLength);

    work->headingCos = rcos(work->relativeHeading);
    work->headingSin = rsin(work->relativeHeading);
    obj->bodyPitch = ShiftFixed12(work->surfacePitch * work->headingCos) +
                     ShiftFixed12(work->camberAngle * work->headingSin);
    obj->bodyRoll = ShiftFixed12(-work->headingCos * work->camberAngle) +
                    ShiftFixed12(work->surfacePitch * work->headingSin);
}

static void UpdateCarTrackProgress(GameCarRuntime *obj, CarTrackWork *work,
                                   s32 alongSegment, s32 lateralOffset) {
    s32 lapProgress;
    s32 sectionProgress;

    obj->trackLateralOffset = lateralOffset;
    obj->progressB = g_RaceSeries != 0
        ? (u32)alongSegment
        : (u32)((s16)work->segmentLength - alongSegment);

    lapProgress = (obj->progressA + obj->progressB) % g_TrackLength;
    obj->trackHeading.value = work->heading;
    obj->previousTrackProgress = obj->trackProgress;
    obj->trackProgress = lapProgress < 0
        ? lapProgress + g_TrackLength
        : lapProgress;

    sectionProgress = g_RaceSeries != 0
        ? g_TrackLength - obj->trackProgress
        : obj->trackProgress;
    obj->trackSection = (s16)(sectionProgress >> 8);
}

s32 UpdateCarTrackState(GameCarRuntime *obj, s32 trackPointIndex, CarTrackLimits *limits) {
    s16 curveMode;
    s32 headingAngle;
    s32 nextPointIndex;
    s32 cosHeading;
    s32 sinHeading;
    s32 arcIndex;
    s32 lateralOffset;
    s32 alongSegment;
    s32 rotated;
    u16 segmentLength;
    GameTrackPoint *point;
    GameTrackPoint *nextPoint;
    CarTrackWork *work;
    static int traceEnabled = -1;
    static int traceTimer = -1;
    static int traceTimerMin = -1;
    static int traceTimerMax = -1;
    int traceThisCall;
    if (traceEnabled < 0) {
        const char *timerText = DiagnosticsValue("car.track_trace_timer");
        const char *timerMinText = DiagnosticsValue("car.track_trace_timer_min");
        const char *timerMaxText = DiagnosticsValue("car.track_trace_timer_max");
        traceEnabled = DiagnosticsEnabled("car.track_trace");
        traceTimer = timerText != NULL ? (int)strtol(timerText, NULL, 0) : -1;
        traceTimerMin = timerMinText != NULL ? (int)strtol(timerMinText, NULL, 0) : -1;
        traceTimerMax = timerMaxText != NULL ? (int)strtol(timerMaxText, NULL, 0) : -1;
    }
    traceThisCall = traceEnabled && obj == (GameCarRuntime *)&g_PlayerCar &&
        (traceTimer < 0 || g_SceneTimer == traceTimer) &&
        (traceTimerMin < 0 || g_SceneTimer >= traceTimerMin) &&
        (traceTimerMax < 0 || g_SceneTimer <= traceTimerMax);
    if (traceThisCall) {
        Trace("car-track-enter", "timer=%d point=%d x=%d z=%d speed=%d "
               "progress=%d lateral=%d yaw=%d limits=%d,%d,%d,%d",
               g_SceneTimer, trackPointIndex, obj->x, obj->z, obj->speed,
               obj->trackProgress, obj->trackLateralOffset, obj->bodyYaw,
               limits->leftInset, limits->rightInset,
               limits->leftKnockbackMode, limits->rightKnockbackMode);
    }

    nextPointIndex = (trackPointIndex + 1) % g_TrackPointCount;
    work = &g_CarTrackWork;
    work->knockbackMode = 0;
    point = TrackPoint(trackPointIndex);
    segmentLength = point->segmentLength;
    work->segmentLength = segmentLength;
    nextPoint = TrackPoint(nextPointIndex);
    if ((s16)segmentLength <= 0) {
        work->segmentLength = 1U;
    }
    work->heading = (u16)point->angle;
    arcIndex = (s16)point->arcRef >> 4;
    work->arcIndex = (s16)arcIndex;
    curveMode = point->arcRef & 3;
    work->curveMode = curveMode;
    if (curveMode != 0) {
        PlaceCarOnArc(obj, work, point, nextPoint, arcIndex);
    }

    work->offsetX = (u16)(((u16)obj->x - (u16)point->x) * 4);
    headingAngle = work->heading;
    work->offsetZ = (s16)(((u16)obj->z - (u16)point->z) * 4);
    work->offsetY = 0;
    cosHeading = rcos(headingAngle);
    rotated = (cosHeading * (s16)work->offsetX) +
              (rsin(work->heading) * work->offsetZ);
    alongSegment = ProjectTrackAxis(rotated);
    sinHeading = rsin(work->heading);
    rotated = (-sinHeading * (s16)work->offsetX) +
              (rcos(work->heading) * work->offsetZ);
    lateralOffset = ProjectTrackAxis(rotated);
    if (work->curveMode != 0) {
        lateralOffset = work->arcLateral;
    }
    lateralOffset = ClampCarToTrackEdges(obj, work, limits, point,
                                         nextPoint, alongSegment,
                                         lateralOffset);
    if ((s16)work->segmentLength < alongSegment) {
        alongSegment = (s16)work->segmentLength;
    } else if (alongSegment < 0) {
        alongSegment = 0;
    }
    obj->segmentFraction = (alongSegment << 0xA) / (s16)work->segmentLength;
    if (lateralOffset < 0) {
        obj->normalizedLateralOffset = (lateralOffset * 0x400) / work->leftHalfWidth;
    } else {
        obj->normalizedLateralOffset = (lateralOffset * 0x400) / work->rightHalfWidth;
    }
    UpdateCarSurfaceOrientation(obj, work, point, nextPoint, alongSegment,
                                lateralOffset);
    UpdateCarTrackProgress(obj, work, alongSegment, lateralOffset);
    if (traceThisCall) {
        Trace("car-track-exit", "timer=%d point=%d x=%d z=%d progress=%d "
               "lateral=%d along=%d heading=%d curve=%d widths=%d,%d "
               "correction=%d,%d knockback=%d motion=%d,%d,%d,%d",
               g_SceneTimer, trackPointIndex, obj->x, obj->z,
               obj->trackProgress, obj->trackLateralOffset, alongSegment,
               work->heading, work->curveMode, work->leftHalfWidth,
               work->rightHalfWidth, work->correctionX, work->correctionZ,
               work->knockbackMode, obj->motionActive, obj->motionTimer,
               obj->velocityX, obj->velocityZ);
    }
    return work->knockbackMode;
}
