#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>

Cd g_Cd;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_Cd.pendingTrack = -1;
    RequestCdTrack(-1);
    CHECK(g_Cd.pendingTrack == -1);
    RequestCdTrack(CD_TRACK_LOCATION_COUNT);
    CHECK(g_Cd.pendingTrack == -1);

    RequestCdTrack(7);
    CHECK(g_Cd.pendingTrack == 7 &&
          g_Cd.trackStep == CD_TRACK_WAIT_FOR_DRIVE);
    CHECK(g_Cd.pendingCommand == CD_COMMAND_NONE &&
          g_Cd.commandStep == CD_PLAY_WAIT_FOR_DRIVE);

    g_Cd.commandStep = 9;
    StartCdAudio();
    CHECK(g_Cd.pendingCommand == CD_COMMAND_PLAY &&
          g_Cd.commandStep == CD_PLAY_WAIT_FOR_DRIVE);

    g_Cd.commandStep = 9;
    PauseCdAudio();
    CHECK(g_Cd.pendingCommand == CD_COMMAND_PAUSE &&
          g_Cd.commandStep == CD_PAUSE_WAIT_FOR_DRIVE);

    g_Cd.restart = 0;
    g_Cd.commandStep = 9;
    ResumeCdAudio();
    CHECK(g_Cd.pendingCommand == CD_COMMAND_PLAY &&
          g_Cd.commandStep == CD_PLAY_WAIT_FOR_DRIVE);

    g_Cd.currentTrack = 5;
    g_Cd.restart = 1;
    g_Cd.trackStep = 0;
    g_Cd.pendingTrack = -1;
    ResumeCdAudio();
    CHECK(g_Cd.pendingTrack == 5 &&
          g_Cd.trackStep == CD_TRACK_RESTART_WAIT_FOR_DRIVE);
    CHECK(g_Cd.pendingCommand == CD_COMMAND_PLAY &&
          g_Cd.commandStep == CD_PLAY_WAIT_FOR_DRIVE);
    CHECK(g_Cd.restart == 0);

    g_Cd.currentTrack = 0xFF;
    g_Cd.restart = 1;
    g_Cd.pendingTrack = -1;
    ResumeCdAudio();
    CHECK(g_Cd.pendingTrack == -1 && g_Cd.restart == 0);
    CHECK(g_Cd.pendingCommand == CD_COMMAND_PLAY);

    g_Cd.currentTrack = 9;
    g_Cd.pendingTrack = 4;
    g_Cd.trackStep = 3;
    g_Cd.pendingCommand = CD_COMMAND_PAUSE;
    g_Cd.commandStep = 6;
    ResetCdAudioState();
    CHECK(g_Cd.currentTrack == 2 && g_Cd.pendingTrack == -1);
    CHECK(g_Cd.trackStep == CD_TRACK_WAIT_FOR_DRIVE &&
          g_Cd.commandStep == CD_PLAY_WAIT_FOR_DRIVE);
    CHECK(g_Cd.pendingCommand == CD_COMMAND_NONE);

    puts("CD audio control tests passed");
    return 0;
}
