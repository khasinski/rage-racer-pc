#ifndef GAME_REPLAY_INTERNAL_H
#define GAME_REPLAY_INTERNAL_H

#include "common.h"
#include "game/replay.h"

struct GameCarRuntime;

typedef union ReplayModelValue {
    s32 word;
    u16 model;
} ReplayModelValue;

extern ReplayGrandPrixFrame *g_ReplayFramesGp;
extern ReplayTimeAttackFrame *g_ReplayFramesTimeAttack;
extern s32 g_ReplayWriteCursor;
extern s32 g_ReplayFrameCount;
extern s32 g_ReplayBufferWrapped;
extern ReplayModelValue g_ReplayPlayerModel;
extern ReplayModelValue g_ReplayRivalModel;

void ApplyReplayFrame(s32 subframe, struct GameCarRuntime *player,
                      struct GameCarRuntime *rival);
void ApplyReplayFrameAndTrackPoint(s32 subframe,
                                   struct GameCarRuntime *player,
                                   struct GameCarRuntime *rival);
void BeginReplay(void);
void UpdateReplayScene(void);
void UpdateReplayFade(void);
void SeedReplayCars(void);
void UpdateReplayCars(void);
void RecordReplayFrame(void);
void ResetReplayFrameCounts(void);
void ResetReplayWriteCursor(void);
void StoreReplayCarFrame(s32 pairIndex, const struct GameCarRuntime *player,
                         const struct GameCarRuntime *rival);
void StoreReplayTimeAttackFrame(s32 pointIndex,
                                const struct GameCarRuntime *player);

#endif
