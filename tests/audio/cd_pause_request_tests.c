#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>
#include <string.h>

CdCommandType g_CdCommandPending;
s32 g_CdCommandStep;
u8 g_CdCurrentTrack;
u8 g_CdLocResult[8];
s32 g_CdRestartOnResume;
CdlLOC g_CdTrackElapsedLoc;
CdlLOC g_CdTrackLoopPoint[18];

static long s_controlResult;
static long s_lastCommand;
static long s_syncResult;

long CdControl(long command, void *param, u_char *result) {
    (void)param;
    (void)result;
    s_lastCommand = command;
    return s_controlResult;
}

long CdSync(long mode, u_char *result) {
    (void)mode;
    (void)result;
    return s_syncResult;
}

long CdPosToInt_Local(CdlLOC *location) {
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
    g_CdCommandStep = CD_PAUSE_WAIT_FOR_DRIVE;
    s_syncResult = 0;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_WAIT_FOR_DRIVE);

    s_syncResult = CD_SYNC_COMPLETE;
    s_controlResult = 1;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_WAIT_FOR_LOCATION &&
          s_lastCommand == CD_DRIVE_GET_LOCATION);

    s_syncResult = CD_SYNC_DISK_ERROR;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_GET_LOCATION);

    g_CdCommandStep = CD_PAUSE_WAIT_FOR_LOCATION;
    s_syncResult = CD_SYNC_COMPLETE;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_CAPTURE_LOCATION);

    g_CdCurrentTrack = 0xFF;
    g_CdRestartOnResume = 1;
    s_controlResult = 1;
    StepCdPauseRequest();
    CHECK(g_CdRestartOnResume == 0 &&
          g_CdCommandStep == CD_PAUSE_WAIT_FOR_COMMAND);

    g_CdCurrentTrack = 1;
    g_CdTrackLoopPoint[0].second = 1;
    g_CdTrackLoopPoint[1].second = 2;
    g_CdLocResult[2] = 0;
    g_CdLocResult[3] = 3;
    s_controlResult = 1;
    g_CdCommandStep = CD_PAUSE_CAPTURE_LOCATION;
    StepCdPauseRequest();
    CHECK(g_CdRestartOnResume == 1);
    CHECK(g_CdTrackElapsedLoc.second == 3 &&
          g_CdTrackElapsedLoc.sector == 0);
    CHECK(g_CdCommandStep == CD_PAUSE_WAIT_FOR_COMMAND &&
          s_lastCommand == CD_DRIVE_PAUSE);

    s_syncResult = CD_SYNC_DISK_ERROR;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_SEND_COMMAND);
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_WAIT_FOR_COMMAND);

    s_syncResult = CD_SYNC_COMPLETE;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_FINISH);
    g_CdCommandPending = CD_COMMAND_PAUSE;
    StepCdPauseRequest();
    CHECK(g_CdCommandStep == CD_PAUSE_WAIT_FOR_DRIVE &&
          g_CdCommandPending == CD_COMMAND_NONE);

    puts("CD pause request tests passed");
    return 0;
}
