#include "game/car_track_internal.h"

static s32 TrackSegmentLength(const TrackRoute *route, s32 index) {
    s32 length = (s16)route->points[index].segmentLength;

    return length > 0 ? length : 0;
}

static s32 OffsetTrackPointIndex(const TrackRoute *route, s32 index, s32 offset) {
    return RouteIndex(route,
        WrapSigned32((int64_t)index + offset));
}

static void AddLapProgress(GameCarRuntime *car, s32 distance) {
    car->progressA = WrapSigned32((int64_t)car->progressA + distance);
}

/*
 * Walks the track-point ring from the event's start point to the car's current
 * point, summing segment lengths into progressA. `seedSelector` picks which way
 * round to walk and the race direction decides the sign of the accumulated
 * distance.
 */
void SeedCarTrackProgress(GameCarRuntime *car, const TrackRoute *route,
                          s32 startIndex, s32 seedSelector, int reverse) {
    s32 current;
    s32 index;
    s32 total = 0;

    if (car == NULL) return;
    if (route == NULL || route->count <= 0 || route->points == NULL) {
        car->progressA = 0;
        return;
    }

    current = RouteIndex(route, car->trackPointIndex);
    index = RouteIndex(route, startIndex);

    if (reverse) {
        if (seedSelector == 1) {
            for (;;) {
                index = RouteIndex(route, index + 1);
                if (index == current) {
                    break;
                }
                total = WrapSigned32(
                    (int64_t)total + TrackSegmentLength(route, index));
            }
        } else {
            for (;;) {
                index = RouteIndex(route, index);
                total = WrapSigned32(
                    (int64_t)total - TrackSegmentLength(route, index));
                if (index == current) {
                    break;
                }
                index--;
            }
        }
    } else {
        if (seedSelector == 0) {
            do {
                index = RouteIndex(route, index + 1);
                total = WrapSigned32(
                    (int64_t)total - TrackSegmentLength(route, index));
            } while (index != current);
        } else {
            while ((index = RouteIndex(route, index)) != current) {
                total = WrapSigned32(
                    (int64_t)total + TrackSegmentLength(route, index));
                index--;
            }
        }
    }
    car->progressA = total;
}


/*
 * Advances trackPointIndex to the supplied segment, walks intervening points and
 * adds or subtracts their segmentLength into car->progressA. The race direction
 * controls which physical direction increases lap progress; equal-length paths
 * keep retail's tie-break (backward for series 0, forward otherwise).
 */
void MoveCarTrackProgress(GameCarRuntime *car, const TrackRoute *route,
                          s32 target, int reverse) {
    s32 current;
    s32 forwardDistance;
    s32 backwardDistance;
    s32 moveForward;
    s32 i;

    if (car == NULL) return;
    if (route == NULL || route->count <= 0 || route->points == NULL) {
        car->activeFlag = -1;
        return;
    }

    current = RouteIndex(route, car->trackPointIndex);
    if (target < 0) {
        car->activeFlag = -1;
        return;
    }
    target = RouteIndex(route, target);
    if (target == current) {
        car->trackPointIndex = current;
        return;
    }

    forwardDistance = target >= current
                          ? target - current
                          : route->count - (current - target);
    backwardDistance = current >= target
                           ? current - target
                           : route->count - (target - current);
    moveForward = forwardDistance < backwardDistance ||
                  (forwardDistance == backwardDistance && reverse);

    /* Forward and backward walks use different endpoints for retail progress.
     * Direction determines which endpoint counts and the sign of its length. */
    s32 steps = moveForward ? forwardDistance : backwardDistance;
    s32 firstOffset = moveForward ? (reverse ? 0 : 1) : (reverse ? -1 : 0);
    s32 stride = moveForward ? 1 : -1;
    s32 sign = moveForward ? (reverse ? 1 : -1) : (reverse ? -1 : 1);
    for (i = 0; i < steps; i++) {
        s32 index = OffsetTrackPointIndex(route, current, firstOffset + stride * i);
        AddLapProgress(car, sign * TrackSegmentLength(route, index));
    }
    car->trackPointIndex = target;
}
