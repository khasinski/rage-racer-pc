#ifndef GAME_REPLAY_INTERNAL_H
#define GAME_REPLAY_INTERNAL_H

#include "common.h"
#include "game/replay.h"

struct GameCarRuntime;

extern s32 g_ReplayWriteCursor;
extern s32 g_ReplayFrameCount;
extern s32 g_ReplayBufferWrapped;
extern s16 g_ReplayPlayerModelIndex;
extern s16 g_ReplayRivalModelIndex;

static inline s32 ReplayFrameCapacity(s32 grandPrixMode) {
    return grandPrixMode != 0 ? GRAND_PRIX_REPLAY_SUBFRAME_COUNT
                              : TIME_ATTACK_REPLAY_SUBFRAME_COUNT;
}

void ApplyReplayFrame(s32 subframe, struct GameCarRuntime *player,
                      struct GameCarRuntime *rival);
void ApplyReplayFrameAndTrackPoint(s32 subframe,
                                   struct GameCarRuntime *player,
                                   struct GameCarRuntime *rival);
void BeginReplay(void);
void UpdateReplayScene(void);
s32 UpdateReplayFade(void);
void DrawSeriesClearedWash(s32 washProgress, s32 fadeLevel);
void SeedReplayCars(void);
void UpdateReplayCars(void);
void RecordReplayFrame(void);
void ResetReplayWriteCursor(void);
s32 ClampReplayFrameCount(s32 frameCount, s32 grandPrixMode);
s32 ReplayEnvironmentRewindTarget(s32 clock, s32 grandPrixMode);
/* Rebuild track-relative progress and model orientation from replay position. */
void ReconstructReplayCarTrackState(struct GameCarRuntime *car);

#endif
