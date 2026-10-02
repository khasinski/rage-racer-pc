/* A finished car's run-out. See web_finish.h. */
#include "web_finish.h"

#include <math.h>
#include <stdint.h>

#include "game/angle.h"

/* car_track_math.c. Prototypes stay local so the coast does not include the
 * physics headers. */
s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment, s16 segmentLength);
s32 CarTrackFixed12ToInteger(s32 value);

static s32 SegmentSpan(const GameTrackPoint *point) {
    s32 length = (s16)point->segmentLength;
    return length > 0 ? length : 1;
}

/* World units per step toward increasing point index. The sign follows the
 * last driving step, so a reverse course and a spun car coast the way they
 * were going. */
static float SpeedAlongRoute(const TrackRoute *route, s32 pointIndex, s32 fraction,
                             float mx, float mz) {
    LVec here, ahead;
    s32 aheadIndex = pointIndex;
    s32 aheadFraction = fraction + 64;
    float fx, fz, len;
    if (aheadFraction > 0x400) {
        aheadFraction -= 0x400;
        aheadIndex = RouteIndex(route, pointIndex + 1);
    }
    InterpolateRoutePoint(route, pointIndex, &here, fraction);
    InterpolateRoutePoint(route, aheadIndex, &ahead, aheadFraction);
    fx = (float)(ahead.x - here.x);
    fz = (float)(ahead.z - here.z);
    len = sqrtf(fx * fx + fz * fz);
    if (!(len > 1.0f)) return 0.0f;
    return (mx * fx + mz * fz) / len;
}

/* `distance` is world units toward increasing point index. */
static void AdvanceRouteDistance(const TrackRoute *route, s32 *pointIndex, s32 *fraction,
                                 float distance) {
    float along, remaining;
    int guard;
    if (!route || route->count <= 0 || distance == 0.0f) return;
    along = (float)(*fraction) * (float)SegmentSpan(RoutePoint(route, *pointIndex)) / 1024.0f;
    remaining = distance;
    for (guard = 0; guard < route->count + 2 && remaining != 0.0f; ++guard) {
        s32 length = SegmentSpan(RoutePoint(route, *pointIndex));
        if (remaining > 0.0f) {
            float room = (float)length - along;
            if (room < 0.0f) room = 0.0f;
            if (remaining <= room) {
                along += remaining;
                remaining = 0.0f;
            } else {
                remaining -= room;
                *pointIndex = RouteIndex(route, *pointIndex + 1);
                along = 0.0f;
            }
        } else if (-remaining <= along) {
            along += remaining;
            remaining = 0.0f;
        } else {
            remaining += along;
            *pointIndex = RouteIndex(route, *pointIndex - 1);
            along = (float)SegmentSpan(RoutePoint(route, *pointIndex));
        }
    }
    {
        s32 length = SegmentSpan(RoutePoint(route, *pointIndex));
        s32 out = length > 0 ? (s32)lroundf(along * 1024.0f / (float)length) : 0;
        if (out < 0) out = 0;
        if (out > 0x400) out = 0x400;
        *fraction = out;
    }
}

/* Race progress shrinks as the point index grows on a normal course
 * (MoveCarTrackProgress). `distance` is toward increasing point index. */
static void AdvanceCoastProgress(FinishRun *run, const TrackRoute *route, int reverse,
                                 float distance) {
    s32 progress;
    if (!route || route->length <= 0) return;
    progress = run->progress + (s32)lroundf(reverse ? distance : -distance);
    progress %= route->length;
    if (progress < 0) progress += route->length;
    run->progress = progress;
}

static s32 CoastSection(const FinishRun *run, const TrackRoute *route, int reverse) {
    s32 section;
    if (!route || route->length <= 0) return 0;
    section = reverse ? route->length - run->progress : run->progress;
    return (s16)(section >> 8);
}

/* Centreline plus the frozen lane offset, sitting on the surface. */
static int SampleCoastPose(const TrackRoute *route, s32 pointIndex, s32 fraction,
                           s32 lateral, s32 yawSlip, PlayerCarRuntime *pose) {
    const GameTrackPoint *point;
    const GameTrackPoint *next;
    s32 length, along, left, right, heading, trackSin, trackCos;
    s32 cross, height, width, surfacePitch, camber;
    s32 nextCamber, pointCamber, cosH, sinH;
    s16 relative;
    LVec center;
    if (!route || !pose || route->count <= 0 || !route->points) return 0;
    if (fraction < 0) fraction = 0;
    if (fraction > 0x400) fraction = 0x400;
    pointIndex = RouteIndex(route, pointIndex);
    point = RoutePoint(route, pointIndex);
    next = RoutePoint(route, pointIndex + 1);
    length = SegmentSpan(point);
    along = (s32)(((int64_t)fraction * length) >> 10);
    if (along < 0) along = 0;
    if (along > length) along = length;
    left = InterpolateCarTrackValue(point->leftHalfWidth, next->leftHalfWidth, along, (s16)length);
    right = InterpolateCarTrackValue(point->rightHalfWidth, next->rightHalfWidth, along, (s16)length);
    if (lateral < -left) lateral = -left;
    if (lateral > right) lateral = right;
    InterpolateRoutePoint(route, pointIndex, &center, fraction);
    heading = InterpolateRouteAngle(route, pointIndex, fraction);
    trackSin = SinAngle(heading);
    trackCos = CosAngle(heading);
    pose->x = WrapSigned32((int64_t)center.x +
                           (((int64_t)-trackSin * lateral) / ANGLE_FULL_TURN));
    pose->z = WrapSigned32((int64_t)center.z +
                           (((int64_t)trackCos * lateral) / ANGLE_FULL_TURN));
    cross = WrapSigned16(InterpolateCarTrackValue(point->crossSlope, next->crossSlope, along, (s16)length));
    height = InterpolateCarTrackValue(point->y, next->y, along, (s16)length);
    pose->y = WrapSigned32((((int64_t)cross * lateral) >> 7) + height);
    pose->modelY = pose->y;
    pose->bodyYaw = (ANGLE_THREE_QUARTER_TURN - heading + yawSlip) & ANGLE_MASK;
    pose->trackPointIndex = pointIndex;
    pose->segmentFraction = fraction;
    pose->trackLateralOffset = lateral;
    relative = WrapSigned16((int64_t)(u16)pose->bodyYaw - ANGLE_THREE_QUARTER_TURN + (u16)heading);
    surfacePitch = WrapSigned16(InterpolateCarTrackValue(
        point->surfacePitch, next->surfacePitch, along, (s16)length));
    width = WrapSigned16((int64_t)(u16)right + (u16)left);
    nextCamber = Atan2(width, (next->crossSlope * width) >> 7);
    pointCamber = Atan2(width, (point->crossSlope * width) >> 7);
    camber = WrapSigned16(InterpolateCarTrackValue(pointCamber, nextCamber, along, (s16)length));
    cosH = CosAngle(relative);
    sinH = SinAngle(relative);
    pose->bodyPitch = CarTrackFixed12ToInteger(surfacePitch * cosH) +
                      CarTrackFixed12ToInteger(camber * sinH);
    pose->bodyRoll = CarTrackFixed12ToInteger(-cosH * camber) +
                     CarTrackFixed12ToInteger(surfacePitch * sinH);
    return 1;
}

static void RememberCoastError(FinishRun *run, const PlayerCarRuntime *simPose,
                               const PlayerCarRuntime *placed, const WebSmooth *smooth) {
    run->errX = (float)(simPose->x - placed->x) + smooth->x;
    run->errY = (float)(simPose->y - placed->y) + smooth->y;
    run->errZ = (float)(simPose->z - placed->z) + smooth->z;
    run->pitchErr = (float)(simPose->bodyPitch - placed->bodyPitch);
    run->rollErr = (float)(simPose->bodyRoll - placed->bodyRoll);
}

static void DecayCoastError(FinishRun *run) {
    const float settle = 0.90f;
    run->errX *= settle;
    run->errY *= settle;
    run->errZ *= settle;
    run->pitchErr *= settle;
    run->rollErr *= settle;
}


void FinishRunReset(FinishRun *run) {
    run->steps = 0;
    run->placed = 0;
    run->along = 0.0f;
    run->errX = run->errY = run->errZ = 0.0f;
    run->pitchErr = run->rollErr = 0.0f;
}

/* At the first step the car's place on the route is snapshot. It then
 * follows the road and brakes; the error between its simulated pose and the
 * lane sample is kept so the car does not jump, and eased out. */
void FinishRunCoast(FinishRun *run, const TrackRoute *route, int reverse,
                    const WebSmooth *smooth, PlayerCarRuntime *pose) {
    int coasting = 0;
    if (!run->placed && route->count > 0 && route->points) {
        s32 routeAngle, aligned;
        PlayerCarRuntime basis;
        run->pointIndex = RouteIndex(route, pose->trackPointIndex);
        run->fraction = pose->segmentFraction;
        if (run->fraction < 0) run->fraction = 0;
        if (run->fraction > 0x400) run->fraction = 0x400;
        run->lateral = pose->trackLateralOffset;
        run->progress = pose->trackProgress;
        routeAngle = InterpolateRouteAngle(route, run->pointIndex, run->fraction);
        aligned = (ANGLE_THREE_QUARTER_TURN - routeAngle) & ANGLE_MASK;
        run->yawSlip = (pose->bodyYaw + (s32)lroundf(smooth->yaw) - aligned) & ANGLE_MASK;
        run->along = SpeedAlongRoute(route, run->pointIndex, run->fraction,
                                     run->motion[0], run->motion[2]);
        basis = *pose;
        if (SampleCoastPose(route, run->pointIndex, run->fraction,
                            run->lateral, run->yawSlip, &basis))
            RememberCoastError(run, pose, &basis, smooth);
        run->placed = 1;
    }
    if (run->steps <= FINISH_COAST_STEPS) {
        /* Full speed at the line, stopped on the last step. */
        const float pace = 1.0f - (float)run->steps / (float)FINISH_COAST_STEPS;
        const float distance = run->along * pace;
        ++run->steps;
        if (run->placed) {
            AdvanceRouteDistance(route, &run->pointIndex, &run->fraction, distance);
            AdvanceCoastProgress(run, route, reverse, distance);
        }
        coasting = 1;
    }
    if (run->placed) {
        PlayerCarRuntime placed = *pose;
        if (SampleCoastPose(route, run->pointIndex, run->fraction,
                            run->lateral, run->yawSlip, &placed)) {
            s32 surfaceY = placed.y;
            s32 y = surfaceY + (s32)lroundf(run->errY);
            pose->x = WrapSigned32((int64_t)placed.x + lroundf(run->errX));
            pose->z = WrapSigned32((int64_t)placed.z + lroundf(run->errZ));
            /* Game Y grows downwards; a value past the surface is in the road. */
            pose->y = y > surfaceY ? surfaceY : y;
            pose->modelY = surfaceY;
            pose->bodyYaw = placed.bodyYaw;
            pose->bodyPitch = WrapSigned32((int64_t)placed.bodyPitch + lroundf(run->pitchErr));
            pose->bodyRoll = WrapSigned32((int64_t)placed.bodyRoll + lroundf(run->rollErr));
            pose->trackPointIndex = placed.trackPointIndex;
            pose->segmentFraction = placed.segmentFraction;
            pose->trackLateralOffset = placed.trackLateralOffset;
            pose->trackProgress = run->progress;
            pose->trackSection = (s16)CoastSection(run, route, reverse);
            CopyPlayerBodyRotationToModel(pose);
        }
    }
    /* This frame still shows the error captured at the line, so the car does
     * not jump onto the centreline sample. Later steps ease that error out
     * and the car settles into the lane. */
    if (coasting) DecayCoastError(run);
}
