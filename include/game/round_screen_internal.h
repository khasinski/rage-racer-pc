#ifndef GAME_ROUND_SCREEN_INTERNAL_H
#define GAME_ROUND_SCREEN_INTERNAL_H

#include "common.h"

typedef struct RoundBgmChoice {
    s32 track;
    s32 shuffleIndex;
} RoundBgmChoice;

s32 DetermineGrandPrixRound(const u8 bestPlaces[4], s32 classIndex,
                            s32 courseIndex);
s32 WrapRoundBgmSelection(s32 selection, s32 trackCount);
RoundBgmChoice ChooseRoundBgm(s32 selection, const u8 *shuffleOrder,
                              s32 trackCount, s32 shuffleIndex);
s32 ClampRoundScreenFade(s32 value);
s32 IsRoundMirrorMode(u16 heldButtons);

#endif
