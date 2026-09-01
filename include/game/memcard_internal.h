#ifndef GAME_MEMCARD_INTERNAL_H
#define GAME_MEMCARD_INTERNAL_H

#include "common.h"

/* Host storage; see the note on g_CourseSelectScrollValue. */
extern s32 g_McConfirmChoice_v;

extern s32 g_FrameSyncThreshold;
extern s32 g_McMenuSubState;
extern s32 GameMenuLoadPhase;

s32 UpdateMemoryCardFade(void);
s32 AdvanceMemoryCardMenuStartup(void);
void DrawMemoryCardMenu(void);

#endif
