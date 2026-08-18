#ifndef GAME_MUSIC_DIRECTOR_H
#define GAME_MUSIC_DIRECTOR_H

#include "common.h"
#include "game/cd.h"

/* Backend-independent CD-DA intent. The recovered globals remain the storage
 * ABI for now; this value object makes transition policy deterministic and
 * independently testable. */
typedef struct AudioSession {
    s32 trackPending;
    s32 trackStep;
    CdCommandType commandPending;
    s32 commandStep;
    u8 currentTrack;
    s32 restartOnResume;
} AudioSession;

void MusicDirectorRequestTrack(AudioSession *state, s32 track);
void MusicDirectorRequestPlay(AudioSession *state);
void MusicDirectorRequestPause(AudioSession *state);
void MusicDirectorRequestResume(AudioSession *state);
void MusicDirectorReset(AudioSession *state);
void MusicDirectorLoopCurrent(AudioSession *state);

#endif
