#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>

CdCommandType g_CdCommandPending;
s32 g_CdCommandStep;
u8 g_CdCurrentTrack;
s32 g_CdRestartOnResume;
s32 g_CdTrackPending;
s32 g_CdTrackStep;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_CdTrackPending = -1;
    RequestCdTrack(-1);
    CHECK(g_CdTrackPending == -1);
    RequestCdTrack(CD_TRACK_LOCATION_COUNT);
    CHECK(g_CdTrackPending == -1);

    RequestCdTrack(7);
    CHECK(g_CdTrackPending == 7 &&
          g_CdTrackStep == CD_TRACK_WAIT_FOR_DRIVE);
    CHECK(g_CdCommandPending == CD_COMMAND_NONE &&
          g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);

    g_CdCommandStep = 9;
    StartCdAudio();
    CHECK(g_CdCommandPending == CD_COMMAND_PLAY &&
          g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);

    g_CdCommandStep = 9;
    PauseCdAudio();
    CHECK(g_CdCommandPending == CD_COMMAND_PAUSE &&
          g_CdCommandStep == CD_PAUSE_WAIT_FOR_DRIVE);

    g_CdRestartOnResume = 0;
    g_CdCommandStep = 9;
    ResumeCdAudio();
    CHECK(g_CdCommandPending == CD_COMMAND_RESUME &&
          g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);

    g_CdCurrentTrack = 5;
    g_CdRestartOnResume = 1;
    g_CdTrackStep = 0;
    g_CdTrackPending = -1;
    ResumeCdAudio();
    CHECK(g_CdTrackPending == 5 &&
          g_CdTrackStep == CD_TRACK_RESTART_WAIT_FOR_DRIVE);
    CHECK(g_CdCommandPending == CD_COMMAND_PLAY &&
          g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);
    CHECK(g_CdRestartOnResume == 0);

    g_CdCurrentTrack = 0xFF;
    g_CdRestartOnResume = 1;
    g_CdTrackPending = -1;
    ResumeCdAudio();
    CHECK(g_CdTrackPending == -1 && g_CdRestartOnResume == 0);
    CHECK(g_CdCommandPending == CD_COMMAND_RESUME);

    g_CdCurrentTrack = 9;
    g_CdTrackPending = 4;
    g_CdTrackStep = 3;
    g_CdCommandPending = CD_COMMAND_PAUSE;
    g_CdCommandStep = 6;
    ResetCdAudioState();
    CHECK(g_CdCurrentTrack == 2 && g_CdTrackPending == -1);
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_DRIVE &&
          g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);
    CHECK(g_CdCommandPending == CD_COMMAND_NONE);

    puts("CD audio control tests passed");
    return 0;
}
