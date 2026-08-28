#include "common.h"
#include "game/diagnostics.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/track.h"
#include "psyq/gte.h"
#include "game/render.h"
#include "game/race.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/track_internal.h"
#include "game/vector.h"

#ifdef __psyz
#include <stdio.h>
#include <stdlib.h>
#endif

typedef union TrackCarAddress {
    PlayerCarRuntime *player;
    GameCarRuntime *runtime;
} TrackCarAddress;

/*
 * Track-segment / route-sprite geometry builder. Interpolates between the
 * GameTrackPoint at `trackPointIndex`
 * and its successor: computes route angles/heights via atan2 (Atan2)
 * and rsin/rcos, builds the collision-boundary
 * offset, and writes the interpolated position/angle/height into the render
 * object `obj`. The scratchpad struct at 0x1F80011C ("spad") is the GTE
 * per-primitive transform scratch. `limits` supplies the boundary margins and
 * knockback modes.
 * Returns the boundary/skid response code.
 */
s32 UpdateCarTrackState(GameCarRuntime *obj, s32 trackPointIndex, CarTrackLimits *limits) {
    s32 headingAngle;
    s32 secondResult;
    s16 segLenE;
    s16 trackWidth;
    s16 trackWidthCopy;
    s16 arcSpan;
    s16 segLenA;
    s16 segLenB;
    s16 segLenC;
    s16 segLenD;
    s32 arcAngle;
    s16 segLenF;
    s16 curveMode;
    s32 sweptAngle;
    s32 swept;
    s32 arcLateral;
    s32 pointHeading;
    s32 carToCenterX;
    s32 pointToCenterX;
    s32 carToCenterZ;
    s32 pointToCenterZ;
    s32 trackLength;
    s32 centerZ;
    s32 forwardComponent;
    s32 lapProgress;
    s32 nextPointIndex;
    s32 edgeHeight;
    s32 surfaceHeight;
    s32 cosCarAngle;
    s32 cosPointAngle;
    s32 cosNextAngle;
    s32 cosHeading;
    s32 sinHeading;
    s32 nextCamber;
    s32 arcCenterZ;
    s32 arcIndex;
    s32 arcCenterX;
    s32 centerX;
    s32 leftLimit;
    s32 rightLimit;
    s32 forwardProduct;
    s32 lateralProduct;
    s32 lateralOffset;
    s32 alongSegment;
    s32 carRadius;
    s32 pointRadius;
    s32 nextRadius;
    s32 rotated;
    u16 segmentLength;
    void *clampSource;
    GameTrackPoint *point;
    GameTrackPoint *nextPoint;
    GameTrackArcCenter *arcCenter;
    CarTrackScratch *spad;
    TrackCarAddress playerAddress;
#ifdef __psyz
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
#endif

    nextPointIndex = (trackPointIndex + 1) % g_TrackPointCount;
    spad = CAR_TRACK_SCRATCH;
    spad->knockbackMode = 0;
    point = TrackPoint(trackPointIndex);
    segmentLength = point->segmentLength;
    spad->segmentLength = segmentLength;
    nextPoint = TrackPoint(nextPointIndex);
    if ((s16)segmentLength <= 0)
    {
        spad->segmentLength = 1U;
    }
    spad->heading = (u16)point->angle;
    arcIndex = (s16)point->arcRef >> 4;
    spad->arcIndex = (s16)arcIndex;
    curveMode = point->arcRef & 3;
    spad->curveMode = curveMode;
    if (curveMode != 0)
    {
        arcCenter = &g_TrackArcCenters[arcIndex];
        arcCenterX = arcCenter->x;
        spad->arcCenterX = arcCenterX;
        arcCenterZ = arcCenter->z;
        spad->arcCenterZ = arcCenterZ;
        carToCenterX = obj->x - arcCenterX;
        spad->carToCenterX = carToCenterX;
        carToCenterZ = obj->z - arcCenterZ;
        spad->carToCenterZ = carToCenterZ;
        spad->sweptAngle = Atan2(carToCenterX, carToCenterZ) & 0xFFF;
        pointToCenterX = point->x;
        centerX = spad->arcCenterX;
        centerZ = spad->arcCenterZ;
        pointToCenterX -= centerX;
        spad->pointToCenterX = pointToCenterX;
        pointToCenterZ = point->z - centerZ;
        spad->pointToCenterZ = pointToCenterZ;
        spad->nextPointToCenterX = nextPoint->x - centerX;
        spad->nextPointToCenterZ = nextPoint->z - centerZ;
        spad->pointAngle = Atan2(pointToCenterX, pointToCenterZ) & 0xFFF;
        spad->nextPointAngle = Atan2(spad->nextPointToCenterX, spad->nextPointToCenterZ) & 0xFFF;
        cosCarAngle = rcos(spad->sweptAngle);
        carRadius = cosCarAngle * spad->carToCenterX + rsin(spad->sweptAngle) * spad->carToCenterZ;
        if (carRadius < 0)
        {
            carRadius += 0xFFF;
        }
        spad->carRadius.value = carRadius >> 0xC;
        cosPointAngle = rcos(spad->pointAngle);
        pointRadius = cosPointAngle * spad->pointToCenterX + rsin(spad->pointAngle) * spad->pointToCenterZ;
        if (pointRadius < 0)
        {
            pointRadius += 0xFFF;
        }
        spad->pointRadius.value = pointRadius >> 0xC;
        cosNextAngle = rcos(spad->nextPointAngle);
        nextRadius = cosNextAngle * spad->nextPointToCenterX + rsin(spad->nextPointAngle) * spad->nextPointToCenterZ;
        if (nextRadius < 0)
        {
            nextRadius += 0xFFF;
        }
        spad->nextPointRadius.value = nextRadius >> 0xC;
        spad->arcSpan = GetAngleDistance(spad->pointAngle, spad->nextPointAngle);
        sweptAngle = GetAngleDistance(spad->pointAngle, spad->sweptAngle);
        arcAngle = spad->arcSpan;
        spad->sweptAngle = sweptAngle;
        {
            s32 interpolated;

            if (arcAngle <= 0)
            {
                interpolated = spad->pointRadius.value;
                spad->arcSpan = 1;
            }
            else
            {
                interpolated = (((s16)sweptAngle * spad->pointRadius.value) +
                                ((arcAngle - (s16)sweptAngle) * spad->nextPointRadius.value)) /
                               arcAngle;
            }
            CAR_TRACK_POINT_RADIUS = interpolated;
        }
        {
            volatile u16 *carRadiusLow = &spad->carRadius.half.low;
            volatile u16 *pointRadiusLow = &spad->pointRadius.half.low;

            arcLateral = (s16)(*carRadiusLow - *pointRadiusLow);
        }
        if (spad->curveMode == 2)
        {
            arcLateral = 0 - arcLateral;
        }
        spad->arcLateral = arcLateral;
        {
            headingAngle = nextPoint->angle;
            pointHeading = point->angle;
            if ((headingAngle - pointHeading) >= 0x801)
            {
                swept = spad->sweptAngle;
                arcSpan = spad->arcSpan;
                spad->heading = (s16)(((headingAngle - 0x1000) * swept +
                                       pointHeading * (arcSpan - swept)) /
                                      arcSpan);
            }
            else if ((pointHeading - headingAngle) >= 0x801)
            {
                swept = spad->sweptAngle;
                arcSpan = spad->arcSpan;
                spad->heading = (s16)((headingAngle * swept +
                                       (pointHeading - 0x1000) * (arcSpan - swept)) /
                                      arcSpan);
            }
            else
            {
                swept = spad->sweptAngle;
                arcSpan = spad->arcSpan;
                spad->heading =
                    (s16)((headingAngle * swept + pointHeading * (arcSpan - swept)) / arcSpan);
            }
        }
    }

    spad->offsetX = (u16)(((u16)obj->x - (u16)point->x) * 4);
    headingAngle = spad->heading;
    spad->offsetZ = (s16)(((u16)obj->z - (u16)point->z) * 4);
    spad->offsetY = 0;
    cosHeading = rcos(headingAngle);
    rotated = (cosHeading * (s16) spad->offsetX) + (rsin(spad->heading) * spad->offsetZ);
    if (rotated < 0)
    {
        rotated += 0xFFF;
    }
    alongSegment = rotated >> 0xE;
    sinHeading = rsin(spad->heading);
    rotated = ((0 - sinHeading) * (s16) spad->offsetX) + (rcos(spad->heading) * spad->offsetZ);
    if (rotated < 0)
    {
        rotated += 0xFFF;
    }
    lateralOffset = rotated >> 0xE;
    if (spad->curveMode != 0)
    {
        lateralOffset = spad->arcLateral;
    }
    segLenA = (s16)spad->segmentLength;
    spad->leftHalfWidth = (s16)((nextPoint->leftHalfWidth * alongSegment +
                           point->leftHalfWidth * (segLenA - alongSegment)) /
                          segLenA);
    segLenB = (s16)spad->segmentLength;
    edgeHeight = (nextPoint->rightHalfWidth * alongSegment +
                  point->rightHalfWidth * (segLenB - alongSegment)) /
                 segLenB;
    spad->rightHalfWidth = (s16) edgeHeight;
    leftLimit = spad->leftHalfWidth + limits->leftInset;
    clampSource = &spad->pad40[0];
    if (lateralOffset < (0 - leftLimit))
    {
        lateralOffset += leftLimit;
        spad->offsetX = 0U;
        spad->offsetY = 0;
        spad->offsetZ = lateralOffset;
        BuildRotMatrixY(clampSource, spad->heading);
        ApplyMatrix(clampSource, &spad->offsetX, &spad->correctionX);
        playerAddress.player = &g_PlayerCar;
        if (obj == playerAddress.runtime)
        {
            SetCarKnockback(obj, spad->correctionX, spad->correctionZ, limits->leftKnockbackMode);
        }
        obj->x = obj->x - spad->correctionX;
        obj->z = obj->z - spad->correctionZ;
        lateralOffset = -spad->leftHalfWidth - limits->leftInset;
        spad->knockbackMode = limits->leftKnockbackMode;
    }
    else
    {
    rightLimit = (s16)edgeHeight - limits->rightInset;
    if (rightLimit < lateralOffset)
    {
        lateralOffset -= rightLimit;
        spad->offsetX = 0U;
        spad->offsetY = 0;
        spad->offsetZ = lateralOffset;
        BuildRotMatrixY(clampSource, spad->heading);
        ApplyMatrix(clampSource, &spad->offsetX, &spad->correctionX);
        playerAddress.player = &g_PlayerCar;
        if (obj == playerAddress.runtime)
        {
            SetCarKnockback(obj, spad->correctionX, spad->correctionZ, limits->rightKnockbackMode);
        }
        obj->x = obj->x - spad->correctionX;
        obj->z = obj->z - spad->correctionZ;
        lateralOffset = spad->rightHalfWidth - limits->rightInset;
        spad->knockbackMode = limits->rightKnockbackMode;
    }
    }
    if ((s16)spad->segmentLength < alongSegment)
    {
        alongSegment = (s16)spad->segmentLength;
    }
    else if (alongSegment < 0)
    {
        alongSegment = 0;
    }
    obj->segmentFraction = (alongSegment << 0xA) / (s16)spad->segmentLength;
    if (lateralOffset < 0)
    {
        obj->normalizedLateralOffset = (lateralOffset * 0x400) / spad->leftHalfWidth;
    }
    else
    {
        obj->normalizedLateralOffset = (lateralOffset * 0x400) / spad->rightHalfWidth;
    }
    {
        u32 outputProgress;
        s32 useProgress;

        useProgress = g_RaceSeries;
        obj->trackLateralOffset = lateralOffset;
        if (useProgress != 0)
        {
            outputProgress = alongSegment;
        }
        else
        {
            outputProgress = (s16)spad->segmentLength - alongSegment;
        }
        obj->progressB = outputProgress;
    }
    segLenC = (s16)spad->segmentLength;
    spad->crossSlope = (s16)((nextPoint->crossSlope * alongSegment +
                           point->crossSlope * (segLenC - alongSegment)) /
                          segLenC);
    segLenD = (s16)spad->segmentLength;
    surfaceHeight =
        (nextPoint->y * alongSegment + point->y * (segLenD - alongSegment)) / segLenD;
    obj->y = surfaceHeight;
    obj->y = ((spad->crossSlope * lateralOffset) >> 7) + surfaceHeight;
    {
        s16 angle;

        angle = (u16)obj->bodyYaw;
        angle -= 0xC00;
        spad->relativeHeading = angle + (u16)spad->heading;
    }
    segLenE = (s16)spad->segmentLength;
    spad->surfacePitch = (s16)((nextPoint->surfacePitch * alongSegment +
                           point->surfacePitch * (segLenE - alongSegment)) /
                          segLenE);
    trackWidth = (u16) spad->rightHalfWidth + (u16) spad->leftHalfWidth;
    spad->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth, (nextPoint->crossSlope * trackWidth) >> 7);
    trackWidthCopy = spad->trackWidth;
    secondResult = Atan2(trackWidthCopy, (point->crossSlope * trackWidthCopy) >> 7);
    segLenF = (s16)spad->segmentLength;
    spad->camberAngle =
        (s16)((nextCamber * alongSegment + secondResult * (segLenF - alongSegment)) / segLenF);
    spad->headingCos = rcos(spad->relativeHeading);
    {
        s32 firstProduct;
        s32 sinValue;
        s32 secondProduct;

        sinValue = rsin(spad->relativeHeading);
        spad->headingSin = sinValue;
        firstProduct = spad->surfacePitch * spad->headingCos;
        if (firstProduct < 0)
        {
            firstProduct += 0xFFF;
        }
        firstProduct >>= 0xC;
        secondProduct = spad->camberAngle * sinValue;
        if (secondProduct < 0)
        {
            secondProduct += 0xFFF;
        }
        obj->bodyPitch = firstProduct + (secondProduct >> 0xC);
    }
    forwardProduct = (0 - spad->headingCos) * spad->camberAngle;
    if (forwardProduct < 0)
    {
        forwardProduct += 0xFFF;
    }
    lateralProduct = spad->surfacePitch * spad->headingSin;
    forwardComponent = forwardProduct >> 0xC;
    if (lateralProduct < 0)
    {
        lateralProduct += 0xFFF;
    }
    trackLength = g_TrackLength;
    lapProgress = (obj->progressA + obj->progressB) % trackLength;
    obj->bodyRoll = forwardComponent + (lateralProduct >> 0xC);
    obj->trackHeading.value = spad->heading;
    obj->previousTrackProgress = obj->trackProgress;
    obj->trackProgress = lapProgress;
    if (lapProgress < 0)
    {
        obj->trackProgress = lapProgress + trackLength;
    }
    {
        s32 finalAngle;

        if (g_RaceSeries != 0)
        {
            finalAngle = g_TrackLength - obj->trackProgress;
            obj->trackSection = (s16)(finalAngle >> 8);
        }
        else
        {
            finalAngle = obj->trackProgress;
            obj->trackSection = (s16)(finalAngle >> 8);
        }
    }
#ifdef __psyz
    if (traceThisCall) {
        Trace("car-track-exit", "timer=%d point=%d x=%d z=%d progress=%d "
               "lateral=%d along=%d heading=%d curve=%d widths=%d,%d "
               "correction=%d,%d knockback=%d motion=%d,%d,%d,%d",
               g_SceneTimer, trackPointIndex, obj->x, obj->z,
               obj->trackProgress, obj->trackLateralOffset, alongSegment,
               spad->heading, spad->curveMode, spad->leftHalfWidth,
               spad->rightHalfWidth, spad->correctionX, spad->correctionZ,
               spad->knockbackMode, obj->motionActive, obj->motionTimer,
               obj->velocityX, obj->velocityZ);
    }
#endif
    return spad->knockbackMode;
}

/*
 * Samples the track surface height under the car. Locates the containing
 * segment (FindTrackSegment), rotates the car position into segment-local space,
 * clamps the along-segment distance `t` to [0, segmentLength], and linearly
 * interpolates the point height `y` and `crossSlope` between the two segment
 * endpoints. Writes the resulting surface height into surfaceY and mirrors it
 * into modelY while vertical motion is idle. The named halfword views preserve
 * the access widths used by this routine.
 */
void SampleTrackSurfaceHeight(CarSurfaceSampleView *car) {
    Matrix mtx;
    SVec v;
    LVec out;
    GameTrackPointHalfwordView *p1;
    GameTrackPointHalfwordView *p2;
    s32 idx;
    s32 seg;
    s32 t;
    s32 oz;
    s32 diff;
    s32 e;
    s32 v8;

    idx = FindTrackSegment(car, car->trackPointIndex);
    p2 = GetTrackPointHalfwordView(
        TrackPoint((idx + 1) % g_TrackPointCount));
    p1 = GetTrackPointHalfwordView(TrackPoint(idx));

    seg = p1->segmentLength;
    v.vx = car->x - p1->x;
    v.vz = car->z - p1->z;
    v.vy = 0;
    BuildRotMatrixY(&mtx, (0x1000 - p1->angle) & 0xFFF);
        ApplyMatrix(&mtx, &v, &out);

    t = out.x;
    oz = out.z;
    if ((s16)seg < t) {
        t = (s16)seg;
    } else if (t < 0) {
        t = 0;
    }

    diff = (s16)seg - t;
    e = (p2->crossSlope * t + p1->crossSlope * diff) / (s16)seg;
    v8 = (p2->y * t + p1->y * diff) / (s16)seg;

    car->surfaceY = ((s16)e * oz >> 7) + v8;
    if (car->verticalMotionState == 0) {
        car->modelY = car->surfaceY;
    }
}
