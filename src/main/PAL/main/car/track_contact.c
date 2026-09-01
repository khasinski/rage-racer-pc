#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

static s32 InterpolateTrackValue(s32 start, s32 end, s32 alongSegment,
                                 s16 segmentLength) {
    return (end * alongSegment + start * (segmentLength - alongSegment)) /
           segmentLength;
}

static s32 ShiftFixed12(s32 value) {
    if (value < 0) {
        value += 0xFFF;
    }
    return value >> 12;
}

static s32 ProjectAlongTrack(s32 value) {
    if (value < 0) {
        value += 0xFFF;
    }
    return value >> 14;
}

static s16 InterpolateTrackHeading(s16 pointHeading, s16 nextHeading,
                                   s32 swept, s16 arcSpan) {
    if (nextHeading - pointHeading >= 0x801) {
        nextHeading -= 0x1000;
    } else if (pointHeading - nextHeading >= 0x801) {
        pointHeading -= 0x1000;
    }
    return (s16)((nextHeading * swept +
                  pointHeading * (arcSpan - swept)) / arcSpan);
}

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
    work->heading = InterpolateTrackHeading(
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
    alongSegment = ProjectAlongTrack(rotated);
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

    work->rightHalfWidth = (s16)InterpolateTrackValue(
        point->rightHalfWidth, nextPoint->rightHalfWidth, alongSegment,
        segmentLength);
    work->leftHalfWidth = (s16)InterpolateTrackValue(
        point->leftHalfWidth, nextPoint->leftHalfWidth, alongSegment,
        segmentLength);
    car->progressB = g_RaceSeries != 0
        ? (u32)alongSegment
        : (u32)(segmentLength - alongSegment);
    work->crossSlope = (s16)InterpolateTrackValue(
        point->crossSlope, nextPoint->crossSlope, alongSegment,
        segmentLength);
    work->surfacePitch = (s16)InterpolateTrackValue(
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
    work->camberAngle = (s16)InterpolateTrackValue(
        pointCamber, nextCamber, alongSegment, segmentLength);
    work->headingCos = rcos(work->relativeHeading);
    work->headingSin = rsin(work->relativeHeading);

    car->modelPitch =
        (work->surfacePitch * work->headingCos) / 4096 +
        ShiftFixed12(work->camberAngle * work->headingSin);
    car->modelRoll =
        ShiftFixed12(-work->headingCos * work->camberAngle) +
        ShiftFixed12(work->surfacePitch * work->headingSin);
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
    s32 pointIndex = car->trackPointIndex;
    const GameTrackPoint *point = TrackPoint(pointIndex);
    const GameTrackPoint *nextPoint =
        TrackPoint((pointIndex + 1) % g_TrackPointCount);
    s32 alongSegment;

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
