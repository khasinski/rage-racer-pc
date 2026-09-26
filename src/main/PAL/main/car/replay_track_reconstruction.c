#include "game/car_track_internal.h"

static void MeasureReplayArc(GameCarRuntime *car, CarTrackWork *work,
                             const GameTrackPoint *point,
                             const GameTrackPoint *nextPoint,
                             const GameTrackArcCenter *arcCenter) {
    s32 sweptAngle;
    s32 sweptContribution;
    s32 remainingContribution;
    s32 lateralOffset;

    CarTrackMeasureArc(work, arcCenter, car->x, car->z, point,
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
    return ClampCarTrackAlongSegment(
        alongSegment, WrapSigned16(work->segmentLength));
}

static void UpdateReplayTrackPosition(GameCarRuntime *car, CarTrackWork *work,
                                      const GameTrackPoint *point,
                                      const GameTrackPoint *nextPoint,
                                      s32 alongSegment, int reverse) {
    s16 segmentLength = WrapSigned16(work->segmentLength);

    work->rightHalfWidth = WrapSigned16(InterpolateCarTrackValue(
        point->rightHalfWidth, nextPoint->rightHalfWidth, alongSegment,
        segmentLength));
    work->leftHalfWidth = WrapSigned16(InterpolateCarTrackValue(
        point->leftHalfWidth, nextPoint->leftHalfWidth, alongSegment,
        segmentLength));
    car->progressB = reverse
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
                                         s32 alongSegment,
                                         const TrackRoute *route, int reverse) {
    s16 segmentLength = WrapSigned16(work->segmentLength);
    s16 trackWidth = WrapSigned16(
        (u16)work->leftHalfWidth + (u16)work->rightHalfWidth);
    s32 nextCamber;
    s32 pointCamber;

    work->relativeHeading = WrapSigned16(
        (u16)car->bodyYaw - ANGLE_THREE_QUARTER_TURN +
        (u16)work->heading);
    work->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth,
                       (nextPoint->crossSlope * trackWidth) >>
                           CAR_TRACK_SURFACE_HEIGHT_SHIFT);
    pointCamber = Atan2(work->trackWidth,
                        (point->crossSlope * work->trackWidth) >>
                            CAR_TRACK_SURFACE_HEIGHT_SHIFT);
    work->camberAngle = WrapSigned16(InterpolateCarTrackValue(
        pointCamber, nextCamber, alongSegment, segmentLength));
    work->headingCos = CosAngle(work->relativeHeading);
    work->headingSin = SinAngle(work->relativeHeading);

    car->modelPitch = WrapSigned16(
        CarTrackFixed12ToInteger(
            work->surfacePitch * work->headingCos) +
        CarTrackFixed12ToInteger(work->camberAngle * work->headingSin));
    car->modelRoll = WrapSigned16(
        CarTrackFixed12ToInteger(-work->headingCos * work->camberAngle) +
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingSin));
    car->modelYaw = car->bodyYaw;
    car->trackHeading = work->heading;
    UpdateCarLapProgressState(car, route->length, reverse);
}

void ReconstructCarTrackState(GameCarRuntime *car, const TrackRoute *route,
                              int reverse) {
    CarTrackWork storage = {0};
    CarTrackWork *work = &storage;
    s32 pointIndex;
    const GameTrackPoint *point;
    const GameTrackPoint *nextPoint;
    s32 alongSegment;

    if (car == NULL || route == NULL || route->count <= 0 ||
        route->points == NULL || route->length <= 0) {
        return;
    }

    pointIndex = car->trackPointIndex;
    point = RoutePoint(route, pointIndex);
    nextPoint = RoutePoint(route, WrapSigned32((int64_t)pointIndex + 1));

    work->trackContact = CAR_TRACK_CONTACT_NONE;
    work->segmentLength = point->segmentLength;
    if (WrapSigned16(work->segmentLength) <= 0) {
        work->segmentLength = 1;
    }
    work->heading = (u16)point->angle;
    work->arcIndex = (s16)TrackPointArcIndex(point);
    work->curveMode = TrackPointCurveMode(point);
    if (work->curveMode != TRACK_CURVE_NONE) {
        if (route->arcs == NULL) return;
        MeasureReplayArc(car, work, point, nextPoint,
                         &route->arcs[work->arcIndex]);
    }

    alongSegment = MeasureAlongSegment(car, work, point);
    UpdateReplayTrackPosition(car, work, point, nextPoint, alongSegment, reverse);
    UpdateReplayTrackOrientation(car, work, point, nextPoint, alongSegment,
                                 route, reverse);
}
