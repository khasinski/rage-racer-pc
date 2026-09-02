#include "common.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>

CdCommandType g_CdCommandPending;
s32 g_CdCommandStep;

static long s_syncResult;
static long s_syncMode;
static long s_controlResult;
static long s_lastCommand;
static s32 s_controlCalls;

long CdSync(long mode, u_char *result) {
    (void)result;
    s_syncMode = mode;
    return s_syncResult;
}

long CdControl(long command, void *param, u_char *result) {
    (void)param;
    (void)result;
    s_lastCommand = command;
    s_controlCalls++;
    return s_controlResult;
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
    g_CdCommandPending = CD_COMMAND_PLAY;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
    s_syncResult = CD_SYNC_COMPLETE;
    s_controlResult = 1;

    StepCdPlayRequest();
    CHECK(g_CdCommandStep == CD_PLAY_WAIT_FOR_COMMAND);
    CHECK(s_syncMode == CD_SYNC_POLL);
    CHECK(s_controlCalls == 1 && s_lastCommand == CD_DRIVE_PLAY);

    s_syncResult = CD_SYNC_DISK_ERROR;
    StepCdPlayRequest();
    CHECK(g_CdCommandStep == CD_PLAY_SEND_COMMAND);

    s_controlResult = 0;
    StepCdPlayRequest();
    CHECK(g_CdCommandStep == CD_PLAY_SEND_COMMAND);
    s_controlResult = 1;
    StepCdPlayRequest();
    CHECK(g_CdCommandStep == CD_PLAY_WAIT_FOR_COMMAND);

    s_syncResult = CD_SYNC_COMPLETE;
    StepCdPlayRequest();
    CHECK(g_CdCommandStep == CD_PLAY_FINISH);
    StepCdPlayRequest();
    CHECK(g_CdCommandPending == CD_COMMAND_NONE);
    CHECK(g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);

    puts("CD play request preserves command retry and completion");
    return 0;
}
