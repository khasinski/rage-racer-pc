#include "game/car_track_internal.h"
#include "game/car_motion_internal.h"

enum {
    MINIMUM_SEGMENT_LENGTH = 1,
    SEGMENT_FRACTION_SHIFT = 10,
};

static void ApplyTrackEdgeCorrection(GameCarRuntime *car,
                                     CarTrackWork *work,
                                     s32 edgePenetration,
                                     CarTrackContact contact, int knockback) {
    work->edgeOffset.vx = 0;
    work->edgeOffset.vy = 0;
    work->edgeOffset.vz = WrapSigned16(edgePenetration);
    /* Only the lateral axis is nonzero. Preserve the old matrix multiply's
     * signed 16-bit inputs and floor rounding without a render/GTE call. */
    work->edgeCorrection.x =
        ((int64_t)WrapSigned16(-SinAngle(work->heading)) *
         work->edgeOffset.vz) >> 12;
    work->edgeCorrection.z =
        ((int64_t)WrapSigned16(CosAngle(work->heading)) *
         work->edgeOffset.vz) >> 12;
    if (knockback) {
        SetTrackBoundaryKnockback(
            car, work->edgeCorrection.x, work->edgeCorrection.z,
            contact);
    }
    car->x = WrapSigned32((int64_t)car->x - work->edgeCorrection.x);
    car->z = WrapSigned32((int64_t)car->z - work->edgeCorrection.z);
    work->trackContact = contact;
}

/*
 * Work out where the track's edges are here, and hold the car inside them.
 *
 * Both half-widths are interpolated along the segment and widened by their own
 * inset; a car outside either is pushed back to the edge and reported as
 * having hit it. Every car is moved back inside; human physics callers request
 * a knockback impulse, while the retail AI requests clamping only.
 */
static s32 ClampCarToTrackEdges(GameCarRuntime *car, CarTrackWork *work,
                                const CarTrackLimits *limits,
                                const GameTrackPoint *point,
                                const GameTrackPoint *nextPoint,
                                s32 alongSegment, s32 lateralOffset, int knockback) {
    s32 leftLimit;
    s32 rightLimit;
    s32 rightHalfWidth;
    s16 segmentLength;

    segmentLength = WrapSigned16(work->segmentLength);
    work->leftHalfWidth = WrapSigned16(InterpolateCarTrackValue(
        point->leftHalfWidth, nextPoint->leftHalfWidth, alongSegment,
        segmentLength));
    rightHalfWidth = InterpolateCarTrackValue(
        point->rightHalfWidth, nextPoint->rightHalfWidth, alongSegment,
        segmentLength);
    work->rightHalfWidth = WrapSigned16(rightHalfWidth);
    leftLimit = work->leftHalfWidth + limits->leftInset;
    if (lateralOffset < -leftLimit) {
        ApplyTrackEdgeCorrection(
            car, work, lateralOffset + leftLimit,
            (CarTrackContact)limits->leftContact, knockback);
        return -leftLimit;
    }
    rightLimit = WrapSigned16(rightHalfWidth) - limits->rightInset;
    if (rightLimit < lateralOffset) {
        ApplyTrackEdgeCorrection(
            car, work, lateralOffset - rightLimit,
            (CarTrackContact)limits->rightContact, knockback);
        lateralOffset = rightLimit;
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
static void PlaceCarOnArc(GameCarRuntime *car, CarTrackWork *work,
                          const GameTrackPoint *point,
                          const GameTrackPoint *nextPoint,
                          const GameTrackArcCenter *arcCenter) {
    s32 arcLateral;
    s32 interpolatedRadius;
    s32 sweptAngle;

    CarTrackMeasureArc(work, arcCenter, car->x, car->z, point, nextPoint);
    work->arcSpan = GetAngleDistance(work->pointAngle, work->nextPointAngle);
    sweptAngle = GetAngleDistance(work->pointAngle, work->sweptAngle);
    work->sweptAngle = sweptAngle;
    if (work->arcSpan <= 0) {
        interpolatedRadius = work->pointRadius.value;
        work->arcSpan = 1;
    } else {
        s32 sweptContribution = WrapSigned32(
            (int64_t)WrapSigned16(sweptAngle) * work->pointRadius.value);
        s32 remainingContribution = WrapSigned32(
            (int64_t)(work->arcSpan - WrapSigned16(sweptAngle)) *
            work->nextPointRadius.value);

        interpolatedRadius = WrapSigned32(
            (int64_t)sweptContribution + remainingContribution) /
            work->arcSpan;
    }
    work->pointRadius.value = interpolatedRadius;
    arcLateral = WrapSigned16(
        (s32)work->carRadius.half.low - work->pointRadius.half.low);
    if (work->curveMode == TRACK_CURVE_MIRRORED) {
        arcLateral = WrapSigned16(-arcLateral);
    }
    work->arcLateral = arcLateral;
    work->heading = InterpolateCarTrackHeading(
        point->angle, nextPoint->angle, work->sweptAngle, work->arcSpan);
}

static void UpdateCarSurfaceOrientation(GameCarRuntime *car,
                                        CarTrackWork *work,
                                        const GameTrackPoint *point,
                                        const GameTrackPoint *nextPoint,
                                        s32 alongSegment,
                                        s32 lateralOffset) {
    s16 segmentLength = WrapSigned16(work->segmentLength);
    s16 trackWidth;
    s32 nextCamber;
    s32 pointCamber;
    s32 surfaceHeight;

    work->crossSlope = WrapSigned16(InterpolateCarTrackValue(
        point->crossSlope, nextPoint->crossSlope, alongSegment,
        segmentLength));
    surfaceHeight = InterpolateCarTrackValue(
        point->y, nextPoint->y, alongSegment, segmentLength);
    car->y = WrapSigned32(
        (int64_t)(WrapSigned32(
            (int64_t)work->crossSlope * lateralOffset) >>
                  CAR_TRACK_SURFACE_HEIGHT_SHIFT) +
        surfaceHeight);

    work->relativeHeading = WrapSigned16(
        (u16)car->bodyYaw - ANGLE_THREE_QUARTER_TURN +
        (u16)work->heading);
    work->surfacePitch = WrapSigned16(InterpolateCarTrackValue(
        point->surfacePitch, nextPoint->surfacePitch, alongSegment,
        segmentLength));

    trackWidth = WrapSigned16(
        (u16)work->rightHalfWidth + (u16)work->leftHalfWidth);
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
    car->bodyPitch =
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingCos) +
        CarTrackFixed12ToInteger(work->camberAngle * work->headingSin);
    car->bodyRoll =
        CarTrackFixed12ToInteger(-work->headingCos * work->camberAngle) +
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingSin);
}

static void UpdateCarTrackProgress(GameCarRuntime *car, CarTrackWork *work,
                                   s32 alongSegment, s32 lateralOffset,
                                   const TrackRoute *route, int reverse) {
    car->trackLateralOffset = lateralOffset;
    car->progressB = reverse
        ? (u32)alongSegment
        : (u32)((s16)work->segmentLength - alongSegment);
    car->trackHeading = work->heading;
    UpdateCarLapProgressState(car, route->length, reverse);
}

static s32 NormalizeLateralOffset(s32 lateralOffset,
                                  const CarTrackWork *work) {
    if (lateralOffset < 0 && work->leftHalfWidth != 0) {
        return WrapSigned32(
                   (int64_t)lateralOffset * ANGLE_QUARTER_TURN) /
               work->leftHalfWidth;
    }
    if (lateralOffset >= 0 && work->rightHalfWidth != 0) {
        return WrapSigned32(
                   (int64_t)lateralOffset * ANGLE_QUARTER_TURN) /
               work->rightHalfWidth;
    }
    return 0;
}

s32 StepCarTrackState(GameCarRuntime *car, const TrackRoute *route,
                      s32 trackPointIndex, const CarTrackLimits *limits,
                      int reverse, int knockback) {
    s32 arcIndex;
    s32 lateralOffset;
    s32 alongSegment;
    const GameTrackPoint *point;
    const GameTrackPoint *nextPoint;
    CarTrackWork storage = {0};
    CarTrackWork *work = &storage;

    if (car == NULL || route == NULL || route->count <= 0 ||
        route->points == NULL || route->length <= 0 || limits == NULL) {
        return 0;
    }

    work->trackContact = CAR_TRACK_CONTACT_NONE;
    point = RoutePoint(route, trackPointIndex);
    nextPoint = RoutePoint(route, WrapSigned32((int64_t)trackPointIndex + 1));
    work->segmentLength = point->segmentLength;
    if (WrapSigned16(work->segmentLength) <= 0) {
        work->segmentLength = MINIMUM_SEGMENT_LENGTH;
    }
    work->heading = (u16)point->angle;
    arcIndex = TrackPointArcIndex(point);
    work->arcIndex = (s16)arcIndex;
    work->curveMode = TrackPointCurveMode(point);
    if (work->curveMode != TRACK_CURVE_NONE) {
        if (route->arcs == NULL) return 0;
        PlaceCarOnArc(car, work, point, nextPoint, &route->arcs[arcIndex]);
    }

    MeasureCarTrackAxes(car, point, work->heading, &work->edgeOffset,
                        &alongSegment, &lateralOffset);
    if (work->curveMode != TRACK_CURVE_NONE) {
        lateralOffset = work->arcLateral;
    }
    lateralOffset = ClampCarToTrackEdges(car, work, limits, point,
                                         nextPoint, alongSegment,
                                         lateralOffset, knockback);
    alongSegment = ClampCarTrackAlongSegment(
        alongSegment, WrapSigned16(work->segmentLength));
    car->segmentFraction = (alongSegment << SEGMENT_FRACTION_SHIFT) /
                           WrapSigned16(work->segmentLength);
    car->normalizedLateralOffset = NormalizeLateralOffset(lateralOffset, work);
    UpdateCarSurfaceOrientation(car, work, point, nextPoint, alongSegment,
                                lateralOffset);
    UpdateCarTrackProgress(car, work, alongSegment, lateralOffset,
                           route, reverse);
    return work->trackContact;
}
