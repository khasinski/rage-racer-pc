#ifndef GAME_MEMCARD_INTERNAL_H
#define GAME_MEMCARD_INTERNAL_H

#include "common.h"

/* Host storage; see the note on g_CourseSelectScrollValue. */
extern volatile s32 g_McConfirmChoice_v;

extern s32 g_FrameSyncThreshold;
extern volatile s32 g_McMenuSubState;
extern volatile s32 GameMenuLoadPhase;

#endif
