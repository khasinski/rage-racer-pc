#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/replay_internal.h"
#include "game/track_internal.h"

static void MeasureReplayArc(GameCarRuntime *car, CarTrackWork *work,
                             const GameTrackPoint *point,
                             const GameTrackPoint *nextPoint) {
    s32 sweptAngle;
    s32 sweptContribution;
    s32 remainingContribution;
    s32 lateralOffset;

    CarTrackMeasureArc(work, work->arcIndex, car->x, car->z, point,
                       nextPoint);
    work->arcSpan = GetAngleDistance(work->pointAngle, work->nextPointAngle);
    if (work->arcSpan <= 0) {
        work->arcSpan = 1;
    }
    sweptAngle = GetAngleDistance(work->pointAngle, work->sweptAngle);
    work->sweptAngle = sweptAngle;
    sweptContribution = WrapSigned32(
        (int64_t)WrapSigned16(sweptAngle) * work->pointRadius.value);
    remainingContribution = WrapSigned32(
        (int64_t)(work->arcSpan - WrapSigned16(sweptAngle)) *
        work->nextPointRadius.value);
    work->pointRadius.value = WrapSigned32(
        (int64_t)sweptContribution + remainingContribution) /
        work->arcSpan;

    lateralOffset = WrapSigned16(
        (s32)work->carRadius.half.low - work->pointRadius.half.low);
    work->arcLateral = work->curveMode == TRACK_CURVE_MIRRORED
        ? WrapSigned16(-lateralOffset)
        : lateralOffset;
    work->heading = InterpolateCarTrackHeading(
        point->angle, nextPoint->angle, work->sweptAngle, work->arcSpan);
}

static s32 MeasureAlongSegment(const GameCarRuntime *car,
                               CarTrackWork *work,
                               const GameTrackPoint *point) {
    s32 alongSegment;

    MeasureCarTrackAxes(car, point, work->heading, &work->edgeOffset,
                        &alongSegment, NULL);
    if (alongSegment > WrapSigned16(work->segmentLength)) {
        return WrapSigned16(work->segmentLength);
    }
    return alongSegment < 0 ? 0 : alongSegment;
}

static void UpdateReplayTrackPosition(GameCarRuntime *car, CarTrackWork *work,
                                      const GameTrackPoint *point,
                                      const GameTrackPoint *nextPoint,
                                      s32 alongSegment) {
    s16 segmentLength = WrapSigned16(work->segmentLength);

    work->rightHalfWidth = WrapSigned16(InterpolateCarTrackValue(
        point->rightHalfWidth, nextPoint->rightHalfWidth, alongSegment,
        segmentLength));
    work->leftHalfWidth = WrapSigned16(InterpolateCarTrackValue(
        point->leftHalfWidth, nextPoint->leftHalfWidth, alongSegment,
        segmentLength));
    car->progressB = g_RaceSeries != 0
        ? (u32)alongSegment
        : (u32)(segmentLength - alongSegment);
    work->crossSlope = WrapSigned16(InterpolateCarTrackValue(
        point->crossSlope, nextPoint->crossSlope, alongSegment,
        segmentLength));
    work->surfacePitch = WrapSigned16(InterpolateCarTrackValue(
        point->surfacePitch, nextPoint->surfacePitch, alongSegment,
        segmentLength));
}

static void UpdateReplayTrackOrientation(GameCarRuntime *car,
                                         CarTrackWork *work,
                                         const GameTrackPoint *point,
                                         const GameTrackPoint *nextPoint,
                                         s32 alongSegment) {
    s16 segmentLength = WrapSigned16(work->segmentLength);
    s16 trackWidth = WrapSigned16(
        (u16)work->leftHalfWidth + (u16)work->rightHalfWidth);
    s32 nextCamber;
    s32 pointCamber;
    s32 progress;

    work->relativeHeading = WrapSigned16(
        (u16)car->bodyYaw - 0xC00 + (u16)work->heading);
    work->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth,
                       (nextPoint->crossSlope * trackWidth) >> 7);
    pointCamber = Atan2(work->trackWidth,
                        (point->crossSlope * work->trackWidth) >> 7);
    work->camberAngle = WrapSigned16(InterpolateCarTrackValue(
        pointCamber, nextCamber, alongSegment, segmentLength));
    work->headingCos = rcos(work->relativeHeading);
    work->headingSin = rsin(work->relativeHeading);

    car->modelPitch = WrapSigned16(
        (work->surfacePitch * work->headingCos) / 4096 +
        CarTrackFixed12ToInteger(work->camberAngle * work->headingSin));
    car->modelRoll = WrapSigned16(
        CarTrackFixed12ToInteger(-work->headingCos * work->camberAngle) +
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingSin));
    car->modelYaw = car->bodyYaw;
    car->trackHeading.value = work->heading;
    car->previousTrackProgress = car->trackProgress;
    progress = CarRaceProgress(car) % g_TrackLength;
    car->trackProgress = progress < 0 ? progress + g_TrackLength : progress;
    car->trackSection = WrapSigned16((g_RaceSeries != 0
        ? g_TrackLength - car->trackProgress
        : car->trackProgress) >> 8);
}

void ReconstructReplayCarTrackState(GameCarRuntime *car) {
    CarTrackWork *work = &g_CarTrackWork;
    s32 pointIndex;
    const GameTrackPoint *point;
    const GameTrackPoint *nextPoint;
    s32 alongSegment;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL ||
        g_TrackLength <= 0) {
        return;
    }

    pointIndex = car->trackPointIndex;
    point = TrackPoint(pointIndex);
    nextPoint = TrackPoint(pointIndex + 1);

    work->knockbackMode = 0;
    work->segmentLength = point->segmentLength;
    if (WrapSigned16(work->segmentLength) <= 0) {
        work->segmentLength = 1;
    }
    work->heading = (u16)point->angle;
    work->arcIndex = (s16)TrackPointArcIndex(point);
    work->curveMode = TrackPointCurveMode(point);
    if (work->curveMode != TRACK_CURVE_NONE) {
        MeasureReplayArc(car, work, point, nextPoint);
    }

    alongSegment = MeasureAlongSegment(car, work, point);
    UpdateReplayTrackPosition(car, work, point, nextPoint, alongSegment);
    UpdateReplayTrackOrientation(car, work, point, nextPoint, alongSegment);
}
