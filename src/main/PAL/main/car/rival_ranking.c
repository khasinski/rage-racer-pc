#include "game/car.h"

enum { RANKED_RIVAL_COUNT = 4 };

void SlowRivalAhead(s32 rank) {
    GameCarRuntime *car;
    GameCarRuntime *rivalAhead;
    s32 progress;
    s32 progressAhead;

    if (rank <= 0 || rank >= RANKED_RIVAL_COUNT ||
        g_RankedCars[rank] == NULL || g_RankedCars[rank - 1] == NULL) {
        return;
    }

    car = g_RankedCars[rank];
    rivalAhead = g_RankedCars[rank - 1];
    progress = car->progressA + car->progressB;
    progressAhead = rivalAhead->progressA + rivalAhead->progressB;

    if (progressAhead - progress >= 0x2800 && rivalAhead->speed >= 0x385) {
        rivalAhead->accelerationLimit =
            (s32)rivalAhead->accelerationLimit * 85 / 100;
    }
}

/* Rank the front four cars by progress. Insertion sort keeps equal-progress
 * cars in stable slot order, which matters while they share the start line. */
void RankContenders(void) {
    s32 progress[RANKED_RIVAL_COUNT];
    s32 indices[RANKED_RIVAL_COUNT] = {0, 1, 2, 3};
    s32 i;

    for (i = 0; i < RANKED_RIVAL_COUNT; i++) {
        progress[i] = g_Cars[i].progressA + g_Cars[i].progressB;
    }

    for (i = 1; i < RANKED_RIVAL_COUNT; i++) {
        s32 index = indices[i];
        s32 position = i;

        while (position > 0 &&
               progress[indices[position - 1]] < progress[index]) {
            indices[position] = indices[position - 1];
            position--;
        }
        indices[position] = index;
    }

    for (i = 0; i < RANKED_RIVAL_COUNT; i++) {
        g_RankedCars[i] = &g_Cars[indices[i]];
    }
}
