#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

static void MeasureReplayArc(GameCarRuntime *car, CarTrackWork *work,
                             const GameTrackPoint *point,
                             const GameTrackPoint *nextPoint) {
    s32 arcAngle;
    s32 sweptAngle;
    s32 lateralOffset;

    CarTrackMeasureArc(work, work->arcIndex, car->x, car->z, point,
                       nextPoint);
    work->arcSpan = GetAngleDistance(work->pointAngle, work->nextPointAngle);
    if (work->arcSpan <= 0) {
        work->arcSpan = 1;
    }
    sweptAngle = GetAngleDistance(work->pointAngle, work->sweptAngle);
    arcAngle = work->arcSpan;
    work->sweptAngle = sweptAngle;
    work->pointRadius.value =
        ((s16)sweptAngle * work->pointRadius.value +
         (arcAngle - (s16)sweptAngle) * work->nextPointRadius.value) /
        arcAngle;

    lateralOffset =
        (s16)(work->carRadius.half.low - work->pointRadius.half.low);
    work->arcLateral = work->curveMode == 2
        ? -lateralOffset
        : lateralOffset;
    work->heading = InterpolateCarTrackHeading(
        point->angle, nextPoint->angle, work->sweptAngle, work->arcSpan);
}

static s32 MeasureAlongSegment(const GameCarRuntime *car,
                               CarTrackWork *work,
                               const GameTrackPoint *point) {
    s32 rotated;
    s32 alongSegment;

    work->offsetX = (u16)(((u16)car->x - (u16)point->x) * 4);
    work->offsetY = 0;
    work->offsetZ = (s16)(((u16)car->z - (u16)point->z) * 4);
    rotated = rcos(work->heading) * (s16)work->offsetX +
              rsin(work->heading) * work->offsetZ;
    alongSegment = ProjectCarTrackAxis(rotated);
    if (alongSegment > (s16)work->segmentLength) {
        return (s16)work->segmentLength;
    }
    return alongSegment < 0 ? 0 : alongSegment;
}

static void UpdateReplayTrackPosition(GameCarRuntime *car, CarTrackWork *work,
                                      const GameTrackPoint *point,
                                      const GameTrackPoint *nextPoint,
                                      s32 alongSegment) {
    s16 segmentLength = (s16)work->segmentLength;

    work->rightHalfWidth = (s16)InterpolateCarTrackValue(
        point->rightHalfWidth, nextPoint->rightHalfWidth, alongSegment,
        segmentLength);
    work->leftHalfWidth = (s16)InterpolateCarTrackValue(
        point->leftHalfWidth, nextPoint->leftHalfWidth, alongSegment,
        segmentLength);
    car->progressB = g_RaceSeries != 0
        ? (u32)alongSegment
        : (u32)(segmentLength - alongSegment);
    work->crossSlope = (s16)InterpolateCarTrackValue(
        point->crossSlope, nextPoint->crossSlope, alongSegment,
        segmentLength);
    work->surfacePitch = (s16)InterpolateCarTrackValue(
        point->surfacePitch, nextPoint->surfacePitch, alongSegment,
        segmentLength);
}

static void UpdateReplayTrackOrientation(GameCarRuntime *car,
                                         CarTrackWork *work,
                                         const GameTrackPoint *point,
                                         const GameTrackPoint *nextPoint,
                                         s32 alongSegment) {
    s16 segmentLength = (s16)work->segmentLength;
    s16 trackWidth = (u16)work->leftHalfWidth +
                     (u16)work->rightHalfWidth;
    s32 nextCamber;
    s32 pointCamber;
    s32 progress;

    work->relativeHeading = (s16)((u16)car->bodyYaw - 0xC00 +
                                  (u16)work->heading);
    work->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth,
                       (nextPoint->crossSlope * trackWidth) >> 7);
    pointCamber = Atan2(work->trackWidth,
                        (point->crossSlope * work->trackWidth) >> 7);
    work->camberAngle = (s16)InterpolateCarTrackValue(
        pointCamber, nextCamber, alongSegment, segmentLength);
    work->headingCos = rcos(work->relativeHeading);
    work->headingSin = rsin(work->relativeHeading);

    car->modelPitch =
        (work->surfacePitch * work->headingCos) / 4096 +
        CarTrackFixed12ToInteger(work->camberAngle * work->headingSin);
    car->modelRoll =
        CarTrackFixed12ToInteger(-work->headingCos * work->camberAngle) +
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingSin);
    car->modelYaw = car->bodyYaw;
    car->trackHeading.value = work->heading;
    car->previousTrackProgress = car->trackProgress;
    progress = (car->progressA + car->progressB) % g_TrackLength;
    car->trackProgress = progress < 0 ? progress + g_TrackLength : progress;
    car->trackSection = (s16)((g_RaceSeries != 0
        ? g_TrackLength - car->trackProgress
        : car->trackProgress) >> 8);
}

void ResetCarTrackState(GameCarRuntime *car) {
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
    if ((s16)work->segmentLength <= 0) {
        work->segmentLength = 1;
    }
    work->heading = (u16)point->angle;
    work->arcIndex = (s16)point->arcRef >> 4;
    work->curveMode = point->arcRef & 3;
    if (work->curveMode != 0) {
        MeasureReplayArc(car, work, point, nextPoint);
    }

    alongSegment = MeasureAlongSegment(car, work, point);
    UpdateReplayTrackPosition(car, work, point, nextPoint, alongSegment);
    UpdateReplayTrackOrientation(car, work, point, nextPoint, alongSegment);
}
