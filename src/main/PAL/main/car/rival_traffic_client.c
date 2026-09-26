#include "game/car_internal.h"
#include "game/player_car_internal.h"
#include "game/rival.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"

void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex) {
    TrafficCar field[RACE_CAR_SLOT_COUNT + 1];
    s32 count = RACE_CAR_SLOT_COUNT;
    for (s32 i = 0; i < count; i++) {
        field[i] = (TrafficCar){&g_Cars[i], 0};
    }
    if (g_SceneId == 0xC) {
        field[count++] = (TrafficCar){AsRivalCar(&g_PlayerCar), 1};
    }
    AvoidRivalTraffic(car, carIndex, g_TrackLength, field, count);
}
