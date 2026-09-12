#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>
#include <string.h>

Cd g_Cd;
CdlLOC g_CdTrackLoopPoint[18];

static long s_controlResult;
static long s_lastCommand;
static long s_syncResult;
static long s_syncMode;

long CdControl(long command, void *param, u_char *result) {
    (void)param;
    (void)result;
    s_lastCommand = command;
    return s_controlResult;
}

long CdSync(long mode, u_char *result) {
    (void)result;
    s_syncMode = mode;
    return s_syncResult;
}

int CdPosToInt(CdlLOC *location) {
    return location->minute * 60 * 75 + location->second * 75 +
           location->sector;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    memset(g_CdTrackLoopPoint, 0, sizeof(g_CdTrackLoopPoint));
    g_Cd.pendingCommand = CD_COMMAND_PAUSE;
    g_Cd.commandStep = 99;
    StepCdPauseRequest();
    CHECK(g_Cd.pendingCommand == CD_COMMAND_NONE &&
          g_Cd.commandStep == CD_PAUSE_WAIT_FOR_DRIVE);

    g_Cd.commandStep = CD_PAUSE_WAIT_FOR_DRIVE;
    s_syncResult = CD_SYNC_PENDING;
    StepCdPauseRequest();
    CHECK(s_syncMode == CD_SYNC_POLL);
    CHECK(g_Cd.commandStep == CD_PAUSE_WAIT_FOR_DRIVE);

    s_syncResult = CD_SYNC_COMPLETE;
    s_controlResult = 1;
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_WAIT_FOR_LOCATION &&
          s_lastCommand == CD_DRIVE_GET_LOCATION);

    s_syncResult = CD_SYNC_DISK_ERROR;
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_GET_LOCATION);

    g_Cd.commandStep = CD_PAUSE_WAIT_FOR_LOCATION;
    s_syncResult = CD_SYNC_COMPLETE;
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_CAPTURE_LOCATION);

    g_Cd.currentTrack = 0xFF;
    g_Cd.restart = 1;
    s_controlResult = 1;
    StepCdPauseRequest();
    CHECK(g_Cd.restart == 0 &&
          g_Cd.commandStep == CD_PAUSE_WAIT_FOR_COMMAND);

    g_Cd.currentTrack = 1;
    g_CdTrackLoopPoint[0].second = 1;
    g_CdTrackLoopPoint[1].second = 2;
    g_Cd.result[2] = 0;
    g_Cd.result[3] = 3;
    s_controlResult = 1;
    g_Cd.commandStep = CD_PAUSE_CAPTURE_LOCATION;
    StepCdPauseRequest();
    CHECK(g_Cd.restart == 1);
    CHECK(g_Cd.elapsed.second == 3 &&
          g_Cd.elapsed.sector == 0);
    CHECK(g_Cd.commandStep == CD_PAUSE_WAIT_FOR_COMMAND &&
          s_lastCommand == CD_DRIVE_PAUSE);

    s_syncResult = CD_SYNC_DISK_ERROR;
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_SEND_COMMAND);
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_WAIT_FOR_COMMAND);

    s_syncResult = CD_SYNC_COMPLETE;
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_FINISH);
    g_Cd.pendingCommand = CD_COMMAND_PAUSE;
    StepCdPauseRequest();
    CHECK(g_Cd.commandStep == CD_PAUSE_WAIT_FOR_DRIVE &&
          g_Cd.pendingCommand == CD_COMMAND_NONE);

    puts("CD pause request tests passed");
    return 0;
}
