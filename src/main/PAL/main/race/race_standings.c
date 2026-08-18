#include "game/race_standings.h"

s32 RaceStandingsCalculatePosition(
    s32 playerProgress,
    const RaceCompetitorProgress *competitors,
    s32 competitorCount) {
    s32 position = 1;
    s32 i;

    for (i = 0; i < competitorCount; i++) {
        if (competitors[i].active &&
            competitors[i].progress > playerProgress) {
            position++;
        }
    }
    return position;
}
