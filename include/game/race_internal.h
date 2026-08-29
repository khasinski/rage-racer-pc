#ifndef GAME_RACE_INTERNAL_H
#define GAME_RACE_INTERNAL_H

#include "common.h"

typedef struct PrologueCameraCut {
    s16 timer;
    s16 carIndex;
} PrologueCameraCut;

typedef struct ResultPlaceBarPosition {
    u8 left;
    u8 right;
} ResultPlaceBarPosition;

typedef struct ResultPlaceSpriteLayout {
    u8 x;
    u8 y;
    u8 width;
} ResultPlaceSpriteLayout;

typedef union SectorReferenceTimes {
    s32 values[3];
    struct {
        s32 first;
        s32 second;
        s32 third;
    } fields;
} SectorReferenceTimes;

extern s32 g_RaceTotalTime;
extern SectorReferenceTimes g_RefSectorTimes;

/* Holds the cue that follows FINISHED until the special voices are free, so a
 * fast host frame cannot make the second cue replace the first. Lives in the
 * race scene; the lap update is what asks for it. */
void QueueFinishFollowupCue(s32 cue);
extern s32 g_PrologueStep;
extern PrologueCameraCut g_PrologueCameraCuts[];
extern ResultPlaceSpriteLayout g_ResultPlaceSprites[];
extern ResultPlaceBarPosition g_ClassPlaceBarSizes[];

#endif
