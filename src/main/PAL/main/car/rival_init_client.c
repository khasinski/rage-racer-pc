#include "game/car_internal.h"
#include "game/rival.h"
#include "game/race.h"
#include "game/track_internal.h"

void InitRivalCar(GameCarRuntime *car, s32 gridPosition, const RaceGridSlot *grid) {
    const s32 reverse = g_RaceSeries != 0;
    const TrackRoute route = {.points = g_TrackPoints, .arcs = g_TrackArcCenters,
        .count = g_TrackPointCount, .length = g_TrackLength};
    InitRival(car, &route, &g_TrackEventData->rivalStarts[reverse][gridPosition + 1],
        g_TrackEventData->trackWalkStart, reverse, RaceGridModelId(grid[gridPosition]));
}
