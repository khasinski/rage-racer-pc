#include "game/car_internal.h"
#include "game/rival.h"
#include "game/race.h"

void SteerCarAlongRoute(GameCarRuntime *car) {
    const TrackRoute route = {.points = g_TrackPoints, .count = g_TrackPointCount};
    SteerRival(car, &route, g_RaceSeries != 0);
}

void ClampCarLateralOffset(GameCarRuntime *car, s32 slot) {
    const TrackRoute route = {.points = g_TrackPoints, .count = g_TrackPointCount};
    ClampRivalLine(car, slot, &route);
}
