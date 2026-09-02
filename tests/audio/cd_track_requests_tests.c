#include "common.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>

CdlLOC g_CdTrackLocs[18];
s32 g_CdTrackPending;
s32 g_CdTrackStep;
u8 g_CdCurrentTrack;
s32 g_CdFadeFrames;
u8 g_CdVolume;

static long s_syncResult;
static long s_controlResult;
static long s_lastCommand;
static void *s_lastParam;
static s32 s_controlCalls;
static s32 s_volumeCalls;
static s32 s_lastVolume;

long CdSync(long mode, u_char *result) {
    (void)mode;
    (void)result;
    return s_syncResult;
}

long CdControl(long command, void *param, u_char *result) {
    (void)result;
    s_lastCommand = command;
    s_lastParam = param;
    s_controlCalls++;
    return s_controlResult;
}

void SetCdVolume(s32 volume) {
    s_volumeCalls++;
    s_lastVolume = volume;
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void ResetCalls(void) {
    s_controlCalls = 0;
    s_volumeCalls = 0;
    s_lastCommand = -1;
    s_lastParam = NULL;
}

static int TestTrackSelection(void) {
    g_CdTrackPending = -9;
    g_CdTrackStep = CD_TRACK_SEND_SEEK;
    ResetCalls();
    StepCdTrackRequest();
    CHECK(g_CdTrackPending == -1 &&
          g_CdTrackStep == CD_TRACK_WAIT_FOR_DRIVE);
    CHECK(s_controlCalls == 0);

    g_CdTrackPending = 5;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    g_CdFadeFrames = 12;
    g_CdVolume = 73;
    ResetCalls();

    s_syncResult = 0;
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_DRIVE && s_controlCalls == 0);

    s_syncResult = CD_SYNC_COMPLETE;
    s_controlResult = 1;
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_PAUSE);
    CHECK(s_lastCommand == CD_DRIVE_PAUSE);

    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_SEEK);
    CHECK(g_CdFadeFrames == 0 && s_lastCommand == CD_DRIVE_SEEK_PLAY);
    CHECK(s_lastParam == &g_CdTrackLocs[5]);

    s_syncResult = CD_SYNC_DISK_ERROR;
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_SEND_SEEK);
    s_controlResult = 0;
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_SEND_SEEK);
    s_controlResult = 1;
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_SEEK);

    s_syncResult = CD_SYNC_COMPLETE;
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_FINISH_SELECTION);
    StepCdTrackRequest();
    CHECK(g_CdCurrentTrack == 5 && g_CdTrackPending == -1);
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_DRIVE);
    CHECK(s_volumeCalls == 1 && s_lastVolume == 73);
    return 0;
}

static int TestTrackRestart(void) {
    g_CdTrackPending = 7;
    g_CdTrackStep = CD_TRACK_RESTART_WAIT_FOR_DRIVE;
    ResetCalls();
    s_syncResult = CD_SYNC_COMPLETE;
    s_controlResult = 1;

    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_RESTART_WAIT_FOR_SEEK);
    CHECK(s_controlCalls == 1 && s_lastCommand == CD_DRIVE_SEEK_PLAY);
    CHECK(s_lastParam == &g_CdTrackLocs[7]);
    StepCdTrackRequest();
    CHECK(g_CdTrackStep == CD_TRACK_FINISH_RESTART);
    StepCdTrackRequest();
    CHECK(g_CdCurrentTrack == 7 && g_CdTrackPending == -1);
    CHECK(g_CdTrackStep == CD_TRACK_WAIT_FOR_DRIVE && s_volumeCalls == 0);
    return 0;
}

int main(void) {
    CHECK(TestTrackSelection() == 0);
    CHECK(TestTrackRestart() == 0);
    puts("CD track requests preserve pause, seek retry, and restart");
    return 0;
}
