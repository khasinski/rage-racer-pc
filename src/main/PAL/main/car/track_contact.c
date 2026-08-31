#include "game/render.h"
#include "game/race.h"
#include "game/car.h"
#include "game/track_internal.h"

/*
 * Rebuilds a car's position and orientation relative to its current track
 * segment, including the curved-segment path kept in the PS1 scratchpad.
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
    CarTrackScratch *spad;

    spad = CAR_TRACK_SCRATCH;
    spad->knockbackMode = 0;
    trackPointIndex = car->trackPointIndex;
    nextPointIndex = (trackPointIndex + 1) % g_TrackPointCount;
    point = TrackPoint(trackPointIndex);
    segmentLength = point->segmentLength;
    spad->segmentLength = segmentLength;
    nextPoint = TrackPoint(nextPointIndex);
    if ((s16)segmentLength <= 0) {
        spad->segmentLength = 1U;
    }
    spad->heading = (u16)point->angle;
    arcIndex = (s16)point->arcRef >> 4;
    spad->arcIndex = (s16)arcIndex;
    curveMode = point->arcRef & 3;
    spad->curveMode = curveMode;
    if (curveMode != 0) {
        CarTrackMeasureArc(spad, arcIndex, car->x, car->z, point,
                           nextPoint);
        spad->arcSpan = GetAngleDistance(spad->pointAngle, spad->nextPointAngle);
        if (spad->arcSpan <= 0) {
            spad->arcSpan = 1;
        }
        sweptAngle =
            GetAngleDistance(spad->pointAngle, spad->sweptAngle);
        arcAngle = spad->arcSpan;
        spad->sweptAngle = sweptAngle;
        spad->pointRadius.value =
            (((s16)sweptAngle * spad->pointRadius.value) +
             ((arcAngle - (s16)sweptAngle) * spad->nextPointRadius.value)) /
            arcAngle;
        arcLateral = (s16)(spad->carRadius.half.low - spad->pointRadius.half.low);
        if (spad->curveMode == 2) {
            arcLateral = 0 - arcLateral;
        }
        spad->arcLateral = arcLateral;
        {
            headingAngle = nextPoint->angle;
            pointHeading = point->angle;
            if ((headingAngle - pointHeading) >= 0x801) {
                swept = spad->sweptAngle;
                arcSpan = spad->arcSpan;
                spad->heading =
                    (s16)((((headingAngle - 0x1000) * swept) +
                           (pointHeading * (arcSpan - swept))) /
                          arcSpan);
            } else if ((pointHeading - headingAngle) >= 0x801) {
                swept = spad->sweptAngle;
                arcSpan = spad->arcSpan;
                spad->heading =
                    (s16)(((headingAngle * swept) +
                           ((pointHeading - 0x1000) * (arcSpan - swept))) /
                          arcSpan);
            } else {
                swept = spad->sweptAngle;
                arcSpan = spad->arcSpan;
                spad->heading =
                    (s16)(((headingAngle * swept) +
                           (pointHeading * (arcSpan - swept))) /
                          arcSpan);
            }
        }
    }

    spad->offsetX = (u16)(((u16)car->x - (u16)point->x) * 4);
    headingAngle = spad->heading;
    spad->offsetZ = (s16)(((u16)car->z - (u16)point->z) * 4);
    spad->offsetY = 0;
    cosHeading = rcos(headingAngle);
    rotated = (cosHeading * (s16)spad->offsetX) +
             (rsin(spad->heading) * spad->offsetZ);
    if (rotated < 0) {
        rotated += 0xFFF;
    }
    alongSegment = rotated >> 0xE;

    if ((s16)spad->segmentLength < alongSegment) {
        alongSegment = (s16)spad->segmentLength;
    } else if (alongSegment < 0) {
        alongSegment = 0;
    }
    segLenA = (s16)spad->segmentLength;
    edgeHeight = ((nextPoint->rightHalfWidth * alongSegment) +
               (point->rightHalfWidth * (segLenA - alongSegment))) /
              segLenA;
    spad->rightHalfWidth = (s16)edgeHeight;
    useProgress = g_RaceSeries;
    segLenB = (s16)spad->segmentLength;
    {
        s32 widthSum;
        s32 remainingLength;

        widthSum = nextPoint->leftHalfWidth * alongSegment;
        remainingLength = segLenB - alongSegment;
        widthSum += point->leftHalfWidth * remainingLength;
        spad->leftHalfWidth = (s16)(widthSum / segLenB);
    }
    {
        u32 outputProgress;

        if (useProgress != 0) {
            outputProgress = alongSegment;
        } else {
            outputProgress = (s16)spad->segmentLength - alongSegment;
        }
        car->progressB = outputProgress;
    }
    segLenC = (s16)spad->segmentLength;
    spad->crossSlope =
        (s16)(((nextPoint->crossSlope * alongSegment) +
               (point->crossSlope * (segLenC - alongSegment))) /
              segLenC);
    {
        s16 angle;

        angle = (u16)car->bodyYaw;
        angle -= 0xC00;
        spad->relativeHeading = angle + (u16)spad->heading;
    }
    segLenD = (s16)spad->segmentLength;
    spad->surfacePitch =
        (s16)(((nextPoint->surfacePitch * alongSegment) +
               (point->surfacePitch * (segLenD - alongSegment))) /
              segLenD);
    trackWidth = (u16)spad->leftHalfWidth + (u16)spad->rightHalfWidth;
    spad->trackWidth = trackWidth;
    nextCamber = Atan2(trackWidth, (nextPoint->crossSlope * trackWidth) >> 7);
    trackWidthCopy = spad->trackWidth;
    secondResult = Atan2(trackWidthCopy, (point->crossSlope * trackWidthCopy) >> 7);
    segLenE = (s16)spad->segmentLength;
    spad->camberAngle =
        (s16)(((nextCamber * alongSegment) +
               (secondResult * (segLenE - alongSegment))) /
              segLenE);
    spad->headingCos = rcos(spad->relativeHeading);
    {
        s32 firstProduct;
        s32 sinValue;
        s32 secondProduct;

        sinValue = rsin(spad->relativeHeading);
        spad->headingSin = sinValue;
        firstProduct = spad->surfacePitch * spad->headingCos;
        firstProduct /= 4096;
        secondProduct = spad->camberAngle * sinValue;
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
                (0 - spad->headingCos) * spad->camberAngle;
            if (firstProduct < 0) {
                firstProduct += 0xFFF;
            }
            lateralProduct = spad->surfacePitch * spad->headingSin;
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
            car->trackHeading.value = spad->heading;
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
