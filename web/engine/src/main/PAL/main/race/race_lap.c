#include "game/race_lap.h"
#include <stddef.h>

LapEvent AdvanceCarLap(PlayerCarRuntime *car, s32 trackLength, s32 lapCount) {
    if (car == NULL || trackLength <= 0 || lapCount < 1 ||
        lapCount > PLAYER_LAP_TIME_CAPACITY || car->lap < 0 ||
        car->lap > PLAYER_LAP_TIME_CAPACITY || car->lap > lapCount) {
        return LAP_NONE;
    }
    const int64_t progress = (int64_t)car->progressA + car->progressB;
    if ((int64_t)car->lap * trackLength > progress) return LAP_NONE;
    car->lap++;
    if (car->lap == 1) return LAP_STARTED;
    return car->lap == lapCount + 1 ? LAP_FINISHED : LAP_COMPLETED;
}

int IsCarLapBehind(const PlayerCarRuntime *car, s32 trackLength) {
    return car != NULL && trackLength > 0 &&
        (int64_t)car->progressA + car->progressB <= -(int64_t)trackLength;
}
