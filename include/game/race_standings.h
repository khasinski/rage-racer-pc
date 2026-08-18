#ifndef GAME_RACE_STANDINGS_H
#define GAME_RACE_STANDINGS_H

#include "common.h"

typedef struct RaceCompetitorProgress {
    s32 progress;
    s32 active;
} RaceCompetitorProgress;

s32 RaceStandingsCalculatePosition(
    s32 playerProgress,
    const RaceCompetitorProgress *competitors,
    s32 competitorCount);

#endif
