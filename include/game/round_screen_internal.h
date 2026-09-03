#ifndef GAME_ROUND_SCREEN_INTERNAL_H
#define GAME_ROUND_SCREEN_INTERNAL_H

#include "common.h"

typedef struct RoundBgmChoice {
    s32 track;
    s32 shuffleIndex;
} RoundBgmChoice;

void EnterRoundScreen(void);
void UpdateRoundScreen(void);
s32 DetermineGrandPrixRound(const u8 bestPlaces[4], s32 classIndex,
                            s32 courseIndex);
s32 RoundScreenTableIndicesValid(s32 series, s32 classIndex,
                                 s32 grandPrixMode);
s32 ClampRoundBgmTrackCount(s32 trackCount);
s32 WrapRoundBgmSelection(s32 selection, s32 trackCount);
RoundBgmChoice ChooseRoundBgm(s32 selection, const u8 *shuffleOrder,
                              s32 trackCount, s32 shuffleIndex);
s32 RoundScreenFadeFromTimer(s32 timer, s32 delay);
s32 NextRoundScreenTimer(s32 timer);
s32 ClampRoundScreenFade(s32 value);
s32 IsRoundMirrorMode(u16 heldButtons);
s32 IsRoundScreenAssetLoadComplete(s32 loadState, s32 loadFailed);

#endif
