#include "game/rival.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    TrackAiSpeedKey table[TRACK_AI_SPEED_KEY_COUNT] = {0};
    for (int i = 0; i < TRACK_AI_SPEED_KEY_COUNT; i++) table[i].progress = 1000;
    table[1].progress = 200;
    table[2].progress = 100; /* Authored reversal must not be sorted away. */
    GameCarRuntime first = {.trackProgress = 150 * 16};
    SeedRivalSpeedKey(&first, table);
    CHECK(first.speedKeyIndex == 2 && first.slideActive == 1);
    const GameCarRuntime saved = first;
    GameCarRuntime second = {.trackProgress = 250 * 16};
    SeedRivalSpeedKey(&second, table);
    CHECK(second.speedKeyIndex == 1);
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    table[0].progress = -1;
    SeedRivalSpeedKey(&second, table);
    CHECK(second.speedKeyIndex == 0);
    for (int i = 0; i < TRACK_AI_SPEED_KEY_COUNT; i++) table[i].progress = 1000;
    table[TRACK_AI_SPEED_KEY_COUNT - 1].progress = 100;
    SeedRivalSpeedKey(&second, table);
    CHECK(second.speedKeyIndex == TRACK_AI_SPEED_KEY_COUNT - 1);
    second.activeFlag = -1;
    const GameCarRuntime inactive = second;
    SeedRivalSpeedKey(&second, table);
    CHECK(memcmp(&second, &inactive, sizeof(second)) == 0);
    SeedRivalSpeedKey(&first, NULL);
    SeedRivalSpeedKey(NULL, table);
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    return 0;
}
