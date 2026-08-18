#ifndef GAME_MUSIC_DIRECTOR_H
#define GAME_MUSIC_DIRECTOR_H

#include "common.h"
#include "game/cd.h"

/* Backend-independent CD-DA intent. The recovered globals remain the storage
 * ABI for now; this value object makes transition policy deterministic and
 * independently testable. */
typedef struct MusicDirectorState {
    s32 trackPending;
    s32 trackStep;
    CdCommandType commandPending;
    s32 commandStep;
    u8 currentTrack;
    s32 restartOnResume;
} MusicDirectorState;

void MusicDirectorRequestTrack(MusicDirectorState *state, s32 track);
void MusicDirectorRequestPlay(MusicDirectorState *state);
void MusicDirectorRequestPause(MusicDirectorState *state);
void MusicDirectorRequestResume(MusicDirectorState *state);
void MusicDirectorReset(MusicDirectorState *state);
void MusicDirectorLoopCurrent(MusicDirectorState *state);

#endif
