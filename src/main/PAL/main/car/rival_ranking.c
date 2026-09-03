#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

enum {
    SLOW_RIVAL_GAP = 0x2800,
    SLOW_RIVAL_MINIMUM_SPEED = 0x385,
    SLOW_RIVAL_ACCELERATION_PERCENT = 85,
};

void SlowRivalAhead(s32 rank) {
    GameCarRuntime *car;
    GameCarRuntime *rivalAhead;
    s32 progress;
    s32 progressAhead;

    if (rank <= 0 || rank >= RIVAL_CONTENDER_COUNT ||
        g_RankedCars[rank] == NULL || g_RankedCars[rank - 1] == NULL) {
        return;
    }

    car = g_RankedCars[rank];
    rivalAhead = g_RankedCars[rank - 1];
    progress = CarRaceProgress(car);
    progressAhead = CarRaceProgress(rivalAhead);

    if (WrapSigned32((int64_t)progressAhead - progress) >= SLOW_RIVAL_GAP &&
        rivalAhead->speed >= SLOW_RIVAL_MINIMUM_SPEED) {
        rivalAhead->accelerationLimit =
            (s32)rivalAhead->accelerationLimit *
            SLOW_RIVAL_ACCELERATION_PERCENT / 100;
    }
}

/* Rank the active front-four cars by progress. Insertion sort keeps
 * equal-progress cars in stable slot order, which matters while they share
 * the start line. Unused rank entries stay null so the rubber-band and cue
 * passes cannot act on disabled grid slots. */
void RankContenders(void) {
    s32 progress[RIVAL_CONTENDER_COUNT];
    s32 indices[RIVAL_CONTENDER_COUNT];
    s32 contenderCount = 0;
    s32 i;

    for (i = 0; i < RIVAL_CONTENDER_COUNT; i++) {
        if (g_Cars[i].activeFlag == -1) {
            continue;
        }
        progress[i] = CarRaceProgress(&g_Cars[i]);
        indices[contenderCount++] = i;
    }

    for (i = 1; i < contenderCount; i++) {
        s32 index = indices[i];
        s32 position = i;

        while (position > 0 &&
               progress[indices[position - 1]] < progress[index]) {
            indices[position] = indices[position - 1];
            position--;
        }
        indices[position] = index;
    }

    for (i = 0; i < contenderCount; i++) {
        g_RankedCars[i] = &g_Cars[indices[i]];
    }
    for (; i < RIVAL_CONTENDER_COUNT; i++) {
        g_RankedCars[i] = NULL;
    }
}
