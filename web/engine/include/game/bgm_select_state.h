#ifndef GAME_BGM_SELECT_STATE_H
#define GAME_BGM_SELECT_STATE_H

#include "common.h"

typedef enum BgmSelectStep {
    BGM_SELECT_STEP_INVALID = -1,
    BGM_SELECT_STEP_LOAD_ASSETS,
    BGM_SELECT_STEP_FADE_IN,
    BGM_SELECT_STEP_ACTIVE,
    BGM_SELECT_STEP_EXIT
} BgmSelectStep;

typedef struct BgmSelect {
    BgmSelectStep step;
    s32 changeDelay;
    s32 cdTrack;
    s32 cursor;
    s32 labelTimer;
    s32 randomPlay;
    s32 showUi;
    s32 track;
} BgmSelect;

#endif
