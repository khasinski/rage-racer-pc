#include "game/car_internal.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track_internal.h"

s32 UpdateCarTrackState(GameCarRuntime *car, s32 pointIndex,
                        const CarTrackLimits *limits) {
    const TrackRoute route = {
        .points = g_TrackPoints,
        .arcs = g_TrackArcCenters,
        .count = g_TrackPointCount,
        .length = g_TrackLength,
    };
    return StepCarTrackState(car, &route, pointIndex, limits,
                             g_RaceSeries != 0,
                             car == AsRivalCar(&g_PlayerCar));
}

s32 FindTrackSegment(const GameCarRuntime *car, s32 startIndex) {
    const TrackRoute route = {
        .points = g_TrackPoints,
        .count = g_TrackPointCount,
    };
    return FindCarTrackSegment(car, &route, startIndex);
}

s32 GetCarCrestTrigger(const GameCarRuntime *car) {
    return FindCarCrest(car, g_TrackEventData, g_TrackLength, g_RaceSeries != 0);
}

void UpdateCarCrestHop(GameCarRuntime *car) {
    StepCarCrestHop(car, g_TrackEventData, g_TrackLength, g_RaceSeries != 0);
}
