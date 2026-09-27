#include "game/rival.h"

/* Preserve authored order: keys can reverse locally and need not terminate. */
void SeedRivalSpeedKey(GameCarRuntime *car,
                         const TrackAiSpeedKey table[TRACK_AI_SPEED_KEY_COUNT]) {
    if (car == NULL || table == NULL || car->activeFlag == -1) return;
    const s32 position = car->trackProgress >> 4;
    car->slideActive = 1;
    car->speedKeyIndex = 0;
    for (s32 index = 0; index < TRACK_AI_SPEED_KEY_COUNT; index++) {
        if (table[index].progress == -1) break;
        if (position >= table[index].progress) {
            car->speedKeyIndex = index;
            break;
        }
    }
}
