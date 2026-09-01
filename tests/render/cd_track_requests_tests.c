#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>
#include <string.h>

CdCommandType g_CdCommandPending;
s32 g_CdCommandStep;
u8 g_CdCurrentTrack;
s32 g_CdFadeFrames;
s32 g_CdTrackPending;
s32 g_CdTrackStep;
u8 g_CdVolume;
CdlLOC g_CdTrackLocs[18];

static s32 s_syncResult;
static s32 s_controlResult;
static s32 s_lastCommand;
static s32 s_volume;

long CdSync(long mode, u_char *result) {
    (void)mode;
    (void)result;
    return s_syncResult;
}

long CdControl(u_char command, u_char *param, u_char *result) {
    (void)param;
    (void)result;
    s_lastCommand = command;
    return s_controlResult;
}

void SetCdVolume(s32 volume) {
    s_volume = volume;
}

static void ResetState(void) {
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdCommandStep = 0;
    g_CdCurrentTrack = 2;
    g_CdFadeFrames = 99;
    g_CdTrackPending = 0x10;
    g_CdTrackStep = 0;
    g_CdVolume = 0x67;
    s_syncResult = 2;
    s_controlResult = 1;
    s_lastCommand = -1;
    s_volume = -1;
    memset(g_CdTrackLocs, 0, sizeof(g_CdTrackLocs));
}

static int TestNewTrackRequest(void) {
    ResetState();
    StepCdTrackRequest();
    if (g_CdTrackStep != 8 || s_lastCommand != 9) return 0;
    StepCdTrackRequest();
    if (g_CdTrackStep != 2 || g_CdFadeFrames != 0 || s_lastCommand != 0x16)
        return 0;
    StepCdTrackRequest();
    if (g_CdTrackStep != 3) return 0;
    StepCdTrackRequest();
    return g_CdTrackStep == 0 && g_CdTrackPending == -1 &&
           g_CdCurrentTrack == 0x10 && s_volume == 0x67;
}

static int TestResumeSeek(void) {
    ResetState();
    g_CdTrackPending = 0x105;
    g_CdTrackStep = 4;
    StepCdTrackRequest();
    if (g_CdTrackStep != 6 || s_lastCommand != 0x16) return 0;
    StepCdTrackRequest();
    if (g_CdTrackStep != 7) return 0;
    StepCdTrackRequest();
    return g_CdTrackStep == 0 && g_CdTrackPending == -1 &&
           g_CdCurrentTrack == 5;
}

int main(void) {
    if (!TestNewTrackRequest() || !TestResumeSeek()) {
        puts("CD track request state machine failed");
        return 1;
    }
    puts("CD track request state machine preserved");
    return 0;
}
