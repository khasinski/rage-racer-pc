#include "common.h"
#include "game/track.h"
#include "psyq/gte.h"
#include "game/render.h"
#include "game/race.h"
#include "game/car.h"
#include "game/scratchpad.h"
#include "game/track_internal.h"
#include "psyq/gpu.h"

s32 GetTrackSurfaceHeight(CarSurfaceSampleView *sample) {
    s32 index;
    s32 nextIndex;
    GameTrackPoint *cur;
    GameTrackPoint *next;
    u32 segmentLengthRaw;
    Matrix mtx;
    s16 vec[4];
    s32 out[3];
    s32 distance;
    s32 segmentLength;
    s32 segmentLengthCompare;
    s32 invDistance;
    s32 outZ;
    s32 fieldE;
    s32 y;
    s32 argX;
    s32 curX;
    s32 argZ;
    s32 curZ;
    s32 angle;

    index = FindTrackSegment(sample, sample->trackPointIndex);
    nextIndex = (index + 1) % g_TrackPointCount;

    cur = &g_TrackPoints[index];

    argX = sample->x;
    curX = (u16)cur->x;
    segmentLengthRaw = cur->segmentLength;
    vec[0] = argX - curX;

    argZ = sample->z;
    curZ = (u16)cur->z;
    vec[1] = 0;
    vec[2] = argZ - curZ;

    angle = cur->angle;
    next = &g_TrackPoints[nextIndex];
    BuildRotMatrixY(&mtx, (0x1000 - angle) & 0xFFF);

    ApplyMatrix(&mtx, vec, out);

    segmentLengthCompare = (s16)segmentLengthRaw;
    distance = out[0];
    outZ = out[2];
    if (segmentLengthCompare < distance) {
        distance = segmentLengthCompare;
    } else if (distance < 0) {
        distance = 0;
    }

    segmentLength = (s16)segmentLengthRaw;
    invDistance = segmentLength - distance;
    fieldE = ((next->crossSlope * distance) + (cur->crossSlope * invDistance)) / segmentLength;
    y = ((next->y * distance) + (cur->y * invDistance)) / segmentLength;

    return y + (((s16)fieldE * outZ) >> 7);
}

/*
 * Rebuilds a car's position and orientation relative to its current track
 * segment, including the curved-segment path kept in the PS1 scratchpad.
 */
void ResetCarTrackState(GameCarRuntime *car) {
    s32 headingAngle;
    s32 secondResult;
    s16 trackWidth;
    s16 trackWidthCopy;
    s32 carToCenterX;
    s32 pointToCenterX;
    s16 arcSpan;
    s16 segLenA;
    s16 segLenB;
    s16 segLenC;
    s16 segLenD;
    s32 carToCenterZ;
    s32 pointToCenterZ;
    s32 arcAngle;
    s16 segLenE;
    s32 centerZ;
    s32 nextPointIndex;
    s32 edgeHeight;
    s32 cosCarAngle;
    s32 cosPointAngle;
    s32 cosNextAngle;
    s32 cosHeading;
    s32 nextCamber;
    s32 arcCenterZ;
    s32 sweptAngle;
    s32 arcIndex;
    s32 arcCenterX;
    s32 centerX;
    s32 swept;
    s32 trackPointIndex;
    s32 useProgress;
    s32 arcLateral;
    s32 lateralProduct;
    s32 pointHeading;
    s32 alongSegment;
    s32 carRadius;
    s32 pointRadius;
    s32 nextRadius;
    s32 rotated;
    s16 curveMode;
    u16 segmentLength;
    GameTrackPoint *point;
    GameTrackPoint *nextPoint;
    GameTrackArcCenter *arcCenter;
    CarTrackScratch *spad;

    spad = CAR_TRACK_SCRATCH;
    spad->knockbackMode = 0;
    trackPointIndex = car->trackPointIndex;
    nextPointIndex = (trackPointIndex + 1) % g_TrackPointCount;
    point = &g_TrackPoints[trackPointIndex];
    segmentLength = point->segmentLength;
    spad->segmentLength = segmentLength;
    nextPoint = &g_TrackPoints[nextPointIndex];
    if ((s16)segmentLength <= 0) {
        spad->segmentLength = 1U;
    }
    spad->heading = (u16)point->angle;
    arcIndex = (s16)point->arcRef >> 4;
    spad->arcIndex = (s16)arcIndex;
    curveMode = point->arcRef & 3;
    spad->curveMode = curveMode;
    if (curveMode != 0) {
        arcCenter = &g_TrackArcCenters[arcIndex];
        arcCenterX = arcCenter->x;
        spad->arcCenterX = arcCenterX;
        arcCenterZ = arcCenter->z;
        spad->arcCenterZ = arcCenterZ;
        carToCenterX = car->x - arcCenterX;
        spad->carToCenterX = carToCenterX;
        carToCenterZ = car->z - arcCenterZ;
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
        carRadius = (cosCarAngle * spad->carToCenterX) +
                 (rsin(spad->sweptAngle) * spad->carToCenterZ);
        if (carRadius < 0) {
            carRadius += 0xFFF;
        }
        spad->carRadius.value = carRadius >> 0xC;
        cosPointAngle = rcos(spad->pointAngle);
        pointRadius = (cosPointAngle * spad->pointToCenterX) +
                   (rsin(spad->pointAngle) * spad->pointToCenterZ);
        if (pointRadius < 0) {
            pointRadius += 0xFFF;
        }
        spad->pointRadius.value = pointRadius >> 0xC;
        cosNextAngle = rcos(spad->nextPointAngle);
        nextRadius = (cosNextAngle * spad->nextPointToCenterX) +
                   (rsin(spad->nextPointAngle) * spad->nextPointToCenterZ);
        if (nextRadius < 0) {
            nextRadius += 0xFFF;
        }
        spad->nextPointRadius.value = nextRadius >> 0xC;
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
        if (firstProduct < 0) {
            firstProduct += 0xFFF;
        }
        firstProduct >>= 0xC;
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
