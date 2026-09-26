#include "game/rival.h"
#include "game/car_track_internal.h"

int AdvanceRival(GameCarRuntime *car, const TrackRoute *route,
                    const TrackEventData *events, s32 slot, int reverse,
                    const TrafficCar *traffic, s32 count) {
    if (car == NULL || route == NULL || route->points == NULL || route->count <= 0 ||
        route->length <= 0 || events == NULL || (u32)slot >= RACE_CAR_SLOT_COUNT ||
        (reverse != 0 && reverse != 1) || count < 0 ||
        count > RACE_CAR_SLOT_COUNT + 1 || (count > 0 && traffic == NULL)) return 0;
    if (car->activeFlag == -1) return 1;
    car->bodyYaw = car->baseBodyYaw;
    if (traffic != NULL) AvoidRivalTraffic(car, slot, route->length, traffic, count);
    StepRivalTargetSpeed(car, slot, events->aiSpeedKeys[reverse], reverse);
    StepRivalLine(car, slot, events->racingLineHints[reverse]);
    ClampRivalLine(car, slot, route);
    SteerRival(car, route, reverse);
    StepRivalAcceleration(car, 1);
    MoveRival(car, slot);
    const s32 segment = FindCarTrackSegment(car, route, car->trackPointIndex);
    MoveCarTrackProgress(car, route, segment, reverse);
    PlaceRival(car, route, reverse);
    return 1;
}
