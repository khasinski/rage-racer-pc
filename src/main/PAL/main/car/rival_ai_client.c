#include "game/car_internal.h"
#include "game/rival.h"
#include "game/race.h"

void InitRivalCarAi(GameCarRuntime *car, s32 gridPosition, const RaceGridSlot *grid) {
    s32 index = grid[gridPosition];
    ConfigureRival(car, g_TrackEventData->rivalAiConfigs[g_RaceSeries != 0], index,
        g_TrackLength, gridPosition);
}
