#include "game/render.h"
#include "game/race.h"
#include "game/car.h"
#include "game/track_internal.h"

/*
 * Rebuilds a car's position and orientation relative to its current track
 * segment, including the curved-segment path kept in the car's track working set.
 */
void ResetCarTrackState(GameCarRuntime *car) {
    s32 headingAngle;
    s32 secondResult;
    s16 trackWidth;
    s16 trackWidthCopy;
    s16 arcSpan;
    s16 segLenA;
    s16 segLenB;
    s16 segLenC;
    s16 segLenD;
    s32 arcAngle;
    s16 segLenE;
    s32 nextPointIndex;
    s32 edgeHeight;
    s32 cosHeading;
    s32 nextCamber;
    s32 sweptAngle;
    s32 arcIndex;
    s32 swept;
    s32 trackPointIndex;
    s32 useProgress;
    s32 arcLateral;
    s32 lateralProduct;
    s32 pointHeading;
    s32 alongSegment;
    s32 rotated;
    s16 curveMode;
    u16 segmentLength;
    GameTrackPoint *point;
    GameTrackPoint *nextPoint;
    CarTrackWork *work;

    work = (&g_CarTrackWork);
    work->knockbackMode = 0;
    trackPointIndex = car->trackPointIndex;
    nextPointIndex = (trackPointIndex + 1) % g_TrackPointCount;
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
        CarTrackMeasureArc(work, arcIndex, car->x, car->z, point,
                           nextPoint);
        work->arcSpan = GetAngleDistance(work->pointAngle, work->nextPointAngle);
        if (work->arcSpan <= 0) {
            work->arcSpan = 1;
        }
        sweptAngle =
            GetAngleDistance(work->pointAngle, work->sweptAngle);
        arcAngle = work->arcSpan;
        work->sweptAngle = sweptAngle;
        work->pointRadius.value =
            (((s16)sweptAngle * work->pointRadius.value) +
             ((arcAngle - (s16)sweptAngle) * work->nextPointRadius.value)) /
            arcAngle;
        arcLateral = (s16)(work->carRadius.half.low - work->pointRadius.half.low);
        if (work->curveMode == 2) {
            arcLateral = 0 - arcLateral;
        }
        work->arcLateral = arcLateral;
        {
            headingAngle = nextPoint->angle;
            pointHeading = point->angle;
            if ((headingAngle - pointHeading) >= 0x801) {
                swept = work->sweptAngle;
                arcSpan = work->arcSpan;
                work->heading =
                    (s16)((((headingAngle - 0x1000) * swept) +
                           (pointHeading * (arcSpan - swept))) /
                          arcSpan);
            } else if ((pointHeading - headingAngle) >= 0x801) {
                swept = work->sweptAngle;
                arcSpan = work->arcSpan;
                work->heading =
                    (s16)(((headingAngle * swept) +
                           ((pointHeading - 0x1000) * (arcSpan - swept))) /
                          arcSpan);
            } else {
                swept = work->sweptAngle;
                arcSpan = work->arcSpan;
                work->heading =
                    (s16)(((headingAngle * swept) +
                           (pointHeading * (arcSpan - swept))) /
                          arcSpan);
            }
        }
    }

    work->offsetX = (u16)(((u16)car->x - (u16)point->x) * 4);
    headingAngle = work->heading;
    work->offsetZ = (s16)(((u16)car->z - (u16)point->z) * 4);
    work->offsetY = 0;
    cosHeading = rcos(headingAngle);
    rotated = (cosHeading * (s16)work->offsetX) +
             (rsin(work->heading) * work->offsetZ);
    if (rotated < 0) {
        rotated += 0xFFF;
    }
    alongSegment = rotated >> 0xE;

    if ((s16)work->segmentLength < alongSegment) {
        alongSegment = (s16)work->segmentLength;
    } else if (alongSegment < 0) {
        alongSegment = 0;
    }
    segLenA = (s16)work->segmentLength;
    edgeHeight = ((nextPoint->rightHalfWidth * alongSegment) +
               (point->rightHalfWidth * (segLenA - alongSegment))) /
              segLenA;
    work->rightHalfWidth = (s16)edgeHeight;
    useProgress = g_RaceSeries;
    segLenB = (s16)work->segmentLength;
    {
        s32 widthSum;
        s32 remainingLength;

        widthSum = nextPoint->leftHalfWidth * alongSegment;
        remainingLength = segLenB - alongSegment;
        widthSum += point->leftHalfWidth * remainingLength;
        work->leftHalfWidth = (s16)(widthSum / segLenB);
    }
    {
        u32 outputProgress;

        if (useProgress != 0) {
            outputProgress = alongSegment;
        } else {
            outputProgress = (s16)work->segmentLength - alongSegment;
        }
        car->progressB = outputProgress;
    }
    segLenC = (s16)work->segmentLength;
    work->crossSlope =
        (s16)(((nextPoint->crossSlope * alongSegment) +
               (point->crossSlope * (segLenC - alongSegment))) /
              segLenC);
    {
        s16 angle;

        angle = (u16)car->bodyYaw;
        angle -= 0xC00;
        work->relativeHeading = angle + (u16)work->heading;
    }
    segLenD = (s16)work->segmentLength;
    work->surfacePitch =
        (s16)(((nextPoint->surfacePitch * alongSegment) +
               (point->surfacePitch * (segLenD - alongSegment))) /
              segLenD);
    trackWidth = (u16)work->leftHalfWidth + (u16)work->rightHalfWidth;
    work->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth, (nextPoint->crossSlope * trackWidth) >> 7);
    trackWidthCopy = work->trackWidth;
    secondResult = Atan2(trackWidthCopy, (point->crossSlope * trackWidthCopy) >> 7);
    segLenE = (s16)work->segmentLength;
    work->camberAngle =
        (s16)(((nextCamber * alongSegment) +
               (secondResult * (segLenE - alongSegment))) /
              segLenE);
    work->headingCos = rcos(work->relativeHeading);
    {
        s32 firstProduct;
        s32 sinValue;
        s32 secondProduct;

        sinValue = rsin(work->relativeHeading);
        work->headingSin = sinValue;
        firstProduct = work->surfacePitch * work->headingCos;
        firstProduct /= 4096;
        secondProduct = work->camberAngle * sinValue;
        if (secondProduct < 0) {
            secondProduct += 0xFFF;
        }
        car->modelPitch = firstProduct + (secondProduct >> 0xC);
    }
    {
        s32 firstComponent;

        {
            s32 firstProduct;

            firstProduct =
                (0 - work->headingCos) * work->camberAngle;
            if (firstProduct < 0) {
                firstProduct += 0xFFF;
            }
            lateralProduct = work->surfacePitch * work->headingSin;
            firstComponent = firstProduct >> 0xC;
        }
        if (lateralProduct < 0) {
            lateralProduct += 0xFFF;
        }
        {
            s32 trackLength;
            s32 progress;
            s32 combinedComponent;

            trackLength = g_TrackLength;
            progress =
                (car->progressA + car->progressB) % trackLength;
            combinedComponent = firstComponent + (lateralProduct >> 0xC);
            car->modelRoll = combinedComponent;
            car->modelYaw = car->bodyYaw;
            car->trackHeading.value = work->heading;
            car->previousTrackProgress = car->trackProgress;
            car->trackProgress = progress;
            if (progress < 0) {
                s32 adjustedProgress;

                adjustedProgress = progress + trackLength;
                car->trackProgress = adjustedProgress;
            }
        }
    }
    {
        s32 finalAngle;

        if (g_RaceSeries != 0) {
            finalAngle = g_TrackLength - car->trackProgress;
            car->trackSection = (s16)(finalAngle >> 8);
        } else {
            finalAngle = car->trackProgress;
            car->trackSection = (s16)(finalAngle >> 8);
        }
    }
}
