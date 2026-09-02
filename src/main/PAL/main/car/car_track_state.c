#include "game/diagnostics.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track_internal.h"

typedef struct {
    int enabled;
    int exactTimer;
    int firstTimer;
    int lastTimer;
} CarTrackTraceConfig;

static int ShouldTraceCarTrackState(const GameCarRuntime *car) {
    static CarTrackTraceConfig config = {-1, -1, -1, -1};

    if (config.enabled < 0) {
        config.enabled = DiagnosticsEnabled("car.track_trace");
        config.exactTimer = DiagnosticsIntValue("car.track_trace_timer", -1);
        config.firstTimer = DiagnosticsIntValue(
            "car.track_trace_timer_min", -1);
        config.lastTimer = DiagnosticsIntValue(
            "car.track_trace_timer_max", -1);
    }

    return config.enabled && car == AsRivalCar(&g_PlayerCar) &&
           (config.exactTimer < 0 || g_SceneTimer == config.exactTimer) &&
           (config.firstTimer < 0 || g_SceneTimer >= config.firstTimer) &&
           (config.lastTimer < 0 || g_SceneTimer <= config.lastTimer);
}

static void TraceCarTrackEnter(const GameCarRuntime *car,
                               s32 trackPointIndex,
                               const CarTrackLimits *limits) {
    Trace("car-track-enter", "timer=%d point=%d x=%d z=%d speed=%d "
          "progress=%d lateral=%d yaw=%d limits=%d,%d,%d,%d",
          g_SceneTimer, trackPointIndex, car->x, car->z, car->speed,
          car->trackProgress, car->trackLateralOffset, car->bodyYaw,
          limits->leftInset, limits->rightInset, limits->leftKnockbackMode,
          limits->rightKnockbackMode);
}

static void TraceCarTrackExit(const GameCarRuntime *car, s32 trackPointIndex,
                              s32 alongSegment,
                              const CarTrackWork *work) {
    Trace("car-track-exit", "timer=%d point=%d x=%d z=%d progress=%d "
          "lateral=%d along=%d heading=%d curve=%d widths=%d,%d "
          "correction=%d,%d knockback=%d motion=%d,%d,%d,%d",
          g_SceneTimer, trackPointIndex, car->x, car->z,
          car->trackProgress, car->trackLateralOffset, alongSegment,
          work->heading, work->curveMode, work->leftHalfWidth,
          work->rightHalfWidth, work->edgeCorrection.x,
          work->edgeCorrection.z,
          work->knockbackMode, car->motionActive, car->motionTimer,
          car->velocityX, car->velocityZ);
}

static void ApplyTrackEdgeCorrection(GameCarRuntime *car,
                                     CarTrackWork *work,
                                     s32 edgePenetration,
                                     s32 knockbackMode) {
    work->edgeOffset.vx = 0;
    work->edgeOffset.vy = 0;
    work->edgeOffset.vz = (s16)edgePenetration;
    BuildRotMatrixY(&work->edgeCorrectionMatrix, work->heading);
    ApplyMatrix(&work->edgeCorrectionMatrix, &work->edgeOffset,
                &work->edgeCorrection);
    if (car == AsRivalCar(&g_PlayerCar)) {
        SetCarKnockback(car, work->edgeCorrection.x,
                        work->edgeCorrection.z, knockbackMode);
    }
    car->x -= work->edgeCorrection.x;
    car->z -= work->edgeCorrection.z;
    work->knockbackMode = knockbackMode;
}

/*
 * Work out where the track's edges are here, and hold the car inside them.
 *
 * Both half-widths are interpolated along the segment and widened by their own
 * inset; a car outside either is pushed back to the edge and reported as
 * having hit it. Every car is moved back inside; only the player additionally
 * receives a knockback impulse.
 */
static s32 ClampCarToTrackEdges(GameCarRuntime *car, CarTrackWork *work,
                                const CarTrackLimits *limits,
                                const GameTrackPoint *point,
                                const GameTrackPoint *nextPoint,
                                s32 alongSegment, s32 lateralOffset) {
    s32 leftLimit;
    s32 rightLimit;
    s32 rightHalfWidth;
    s16 segmentLength;

    segmentLength = (s16)work->segmentLength;
    work->leftHalfWidth = (s16)InterpolateCarTrackValue(
        point->leftHalfWidth, nextPoint->leftHalfWidth, alongSegment,
        segmentLength);
    rightHalfWidth = InterpolateCarTrackValue(
        point->rightHalfWidth, nextPoint->rightHalfWidth, alongSegment,
        segmentLength);
    work->rightHalfWidth = (s16)rightHalfWidth;
    leftLimit = work->leftHalfWidth + limits->leftInset;
    if (lateralOffset < -leftLimit) {
        ApplyTrackEdgeCorrection(car, work, lateralOffset + leftLimit,
                                 limits->leftKnockbackMode);
        return -leftLimit;
    }
    rightLimit = (s16)rightHalfWidth - limits->rightInset;
    if (rightLimit < lateralOffset) {
        ApplyTrackEdgeCorrection(car, work, lateralOffset - rightLimit,
                                 limits->rightKnockbackMode);
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
                          const GameTrackPoint *nextPoint, s32 arcIndex) {
    s32 arcLateral;
    s32 interpolatedRadius;
    s32 sweptAngle;

    CarTrackMeasureArc(work, arcIndex, car->x, car->z, point, nextPoint);
    work->arcSpan = GetAngleDistance(work->pointAngle, work->nextPointAngle);
    sweptAngle = GetAngleDistance(work->pointAngle, work->sweptAngle);
    work->sweptAngle = sweptAngle;
    if (work->arcSpan <= 0) {
        interpolatedRadius = work->pointRadius.value;
        work->arcSpan = 1;
    } else {
        interpolatedRadius = (((s16)sweptAngle * work->pointRadius.value) +
                              ((work->arcSpan - (s16)sweptAngle) *
                               work->nextPointRadius.value)) /
                             work->arcSpan;
    }
    work->pointRadius.value = interpolatedRadius;
    arcLateral = (s16)(work->carRadius.half.low - work->pointRadius.half.low);
    if (work->curveMode == TRACK_CURVE_MIRRORED) {
        arcLateral = -arcLateral;
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
    s16 segmentLength = (s16)work->segmentLength;
    s16 trackWidth;
    s32 nextCamber;
    s32 pointCamber;
    s32 surfaceHeight;

    work->crossSlope = (s16)InterpolateCarTrackValue(
        point->crossSlope, nextPoint->crossSlope, alongSegment, segmentLength);
    surfaceHeight = InterpolateCarTrackValue(
        point->y, nextPoint->y, alongSegment, segmentLength);
    car->y = ((work->crossSlope * lateralOffset) >> 7) + surfaceHeight;

    work->relativeHeading = (s16)((u16)car->bodyYaw - 0xC00 +
                                  (u16)work->heading);
    work->surfacePitch = (s16)InterpolateCarTrackValue(
        point->surfacePitch, nextPoint->surfacePitch, alongSegment,
        segmentLength);

    trackWidth = (u16)work->rightHalfWidth + (u16)work->leftHalfWidth;
    work->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth,
                       (nextPoint->crossSlope * trackWidth) >> 7);
    pointCamber = Atan2(work->trackWidth,
                        (point->crossSlope * work->trackWidth) >> 7);
    work->camberAngle = (s16)InterpolateCarTrackValue(
        pointCamber, nextCamber, alongSegment, segmentLength);

    work->headingCos = rcos(work->relativeHeading);
    work->headingSin = rsin(work->relativeHeading);
    car->bodyPitch =
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingCos) +
        CarTrackFixed12ToInteger(work->camberAngle * work->headingSin);
    car->bodyRoll =
        CarTrackFixed12ToInteger(-work->headingCos * work->camberAngle) +
        CarTrackFixed12ToInteger(work->surfacePitch * work->headingSin);
}

static void UpdateCarTrackProgress(GameCarRuntime *car, CarTrackWork *work,
                                   s32 alongSegment, s32 lateralOffset) {
    s32 lapProgress;
    s32 sectionProgress;

    car->trackLateralOffset = lateralOffset;
    car->progressB = g_RaceSeries != 0
        ? (u32)alongSegment
        : (u32)((s16)work->segmentLength - alongSegment);

    lapProgress = (car->progressA + car->progressB) % g_TrackLength;
    car->trackHeading.value = work->heading;
    car->previousTrackProgress = car->trackProgress;
    car->trackProgress = lapProgress < 0
        ? lapProgress + g_TrackLength
        : lapProgress;

    sectionProgress = g_RaceSeries != 0
        ? g_TrackLength - car->trackProgress
        : car->trackProgress;
    car->trackSection = (s16)(sectionProgress >> 8);
}

static void MeasureCarSegmentPosition(const GameCarRuntime *car,
                                      CarTrackWork *work,
                                      const GameTrackPoint *point,
                                      s32 *alongSegment,
                                      s32 *lateralOffset) {
    s32 headingSin;
    s32 headingCos;

    work->edgeOffset.vx =
        (s16)(u16)(((u16)car->x - (u16)point->x) * 4);
    work->edgeOffset.vy = 0;
    work->edgeOffset.vz =
        (s16)(((u16)car->z - (u16)point->z) * 4);
    headingSin = rsin(work->heading);
    headingCos = rcos(work->heading);
    *alongSegment = ProjectCarTrackAxis(
        headingCos * work->edgeOffset.vx +
        headingSin * work->edgeOffset.vz);
    *lateralOffset = ProjectCarTrackAxis(
        -headingSin * work->edgeOffset.vx +
        headingCos * work->edgeOffset.vz);
}

static s32 ClampAlongSegment(s32 alongSegment, s16 segmentLength) {
    if (alongSegment < 0) return 0;
    if (alongSegment > segmentLength) return segmentLength;
    return alongSegment;
}

static s32 NormalizeLateralOffset(s32 lateralOffset,
                                  const CarTrackWork *work) {
    if (lateralOffset < 0 && work->leftHalfWidth != 0)
        return lateralOffset * 0x400 / work->leftHalfWidth;
    if (lateralOffset >= 0 && work->rightHalfWidth != 0)
        return lateralOffset * 0x400 / work->rightHalfWidth;
    return 0;
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 trackPointIndex,
                        const CarTrackLimits *limits) {
    s32 arcIndex;
    s32 lateralOffset;
    s32 alongSegment;
    GameTrackPoint *point;
    GameTrackPoint *nextPoint;
    CarTrackWork *work;
    int traceThisCall;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL ||
        g_TrackLength <= 0 || limits == NULL) {
        return 0;
    }

    traceThisCall = ShouldTraceCarTrackState(car);
    if (traceThisCall) {
        TraceCarTrackEnter(car, trackPointIndex, limits);
    }

    work = &g_CarTrackWork;
    work->knockbackMode = 0;
    point = TrackPoint(trackPointIndex);
    nextPoint = TrackPoint(trackPointIndex + 1);
    work->segmentLength = point->segmentLength;
    if ((s16)work->segmentLength <= 0) work->segmentLength = 1;
    work->heading = (u16)point->angle;
    arcIndex = TrackPointArcIndex(point);
    work->arcIndex = (s16)arcIndex;
    work->curveMode = TrackPointCurveMode(point);
    if (work->curveMode != TRACK_CURVE_NONE) {
        PlaceCarOnArc(car, work, point, nextPoint, arcIndex);
    }

    MeasureCarSegmentPosition(car, work, point, &alongSegment,
                              &lateralOffset);
    if (work->curveMode != TRACK_CURVE_NONE) {
        lateralOffset = work->arcLateral;
    }
    lateralOffset = ClampCarToTrackEdges(car, work, limits, point,
                                         nextPoint, alongSegment,
                                         lateralOffset);
    alongSegment = ClampAlongSegment(alongSegment,
                                     (s16)work->segmentLength);
    car->segmentFraction = (alongSegment << 0xA) / (s16)work->segmentLength;
    car->normalizedLateralOffset = NormalizeLateralOffset(lateralOffset, work);
    UpdateCarSurfaceOrientation(car, work, point, nextPoint, alongSegment,
                                lateralOffset);
    UpdateCarTrackProgress(car, work, alongSegment, lateralOffset);
    if (traceThisCall) {
        TraceCarTrackExit(car, trackPointIndex, alongSegment, work);
    }
    return work->knockbackMode;
}
