#include "common.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>
#include <string.h>

CdCommandType g_CdCommandPending;
s32 g_CdCommandStep;
u8 g_CdCurrentTrack;
s32 g_CdFadeFrames;
s32 g_CdMixPreset;
s32 g_CdRestartOnResume;
s32 g_CdTrackPending;
s32 g_CdTrackStep;
u8 g_CdVolume;
u8 g_CdModeParam;
s32 g_CdTrackEnded;
s32 g_SceneId;
CdlLOC g_CdTrackLoopPoint[18];

static s32 s_trackSteps;
static s32 s_playSteps;
static s32 s_pauseSteps;
static s32 s_fadeSteps;
static s32 s_hostEnded;
static s32 s_buildCalls;
static s32 s_setVolume;
static s32 s_cdCommand;
static s32 s_spuInputCalls;
static s32 s_serialVolumeCalls;

void StepCdTrackRequest(void) { s_trackSteps++; }
void StepCdPlayRequest(void) { s_playSteps++; }
void StepCdPauseRequest(void) { s_pauseSteps++; }
void StepCdVolumeFade(void) { s_fadeSteps++; }
s32 HostCdAudioEnded(void) { return s_hostEnded; }
void BuildCdTrackTable(void) { s_buildCalls++; }
void SetCdVolume(s32 volume) { s_setVolume = volume; }

void SsSetSpuInputAttr(u8 arg0, u8 arg1, u8 arg2) {
    if (arg0 == 0 && arg1 == 0 && arg2 == 1) {
        s_spuInputCalls++;
    }
}

void SsSetSerialVol(u8 channel, short left, short right) {
    if (channel == 0 && left == 0x7fff && right == 0x7fff) {
        s_serialVolumeCalls++;
    }
}

long CdControl(long command, void *param, u_char *result) {
    (void)param;
    (void)result;
    s_cdCommand = (s32)command;
    return 1;
}

int CdPosToInt(CdlLOC *location) {
    return location->minute * 60 * 75 + location->second * 75 +
           location->sector;
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void Reset(void) {
    memset(g_CdTrackLoopPoint, 0, sizeof(g_CdTrackLoopPoint));
    g_CdTrackPending = -1;
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdCurrentTrack = 3;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
    g_CdTrackEnded = 0;
    g_SceneId = 0;
    s_trackSteps = 0;
    s_playSteps = 0;
    s_pauseSteps = 0;
    s_fadeSteps = 0;
    s_hostEnded = 0;
}

static int TestInitialization(void) {
    s_buildCalls = 0;
    s_setVolume = -1;
    s_cdCommand = -1;
    s_spuInputCalls = 0;
    s_serialVolumeCalls = 0;
    InitCdAudio();
    CHECK(s_spuInputCalls == 1 && s_serialVolumeCalls == 1);
    CHECK(s_cdCommand == CD_DRIVE_SET_MODE &&
          g_CdModeParam ==
              (CD_MODE_CDDA | CD_MODE_AUTO_PAUSE | CD_MODE_REPORT));
    CHECK(s_buildCalls == 1 && s_setVolume == 127);
    CHECK(g_CdTrackPending == -1 && g_CdCommandPending == CD_COMMAND_NONE);
    CHECK(g_CdCurrentTrack == 2 && g_CdVolume == 127);
    return 0;
}

static int TestRequestDispatch(void) {
    Reset();
    g_CdTrackPending = 4;
    g_CdCommandPending = CD_COMMAND_PAUSE;
    TickCdAudio();
    CHECK(s_trackSteps == 1 && s_pauseSteps == 0 && s_fadeSteps == 1);

    Reset();
    g_CdCommandPending = CD_COMMAND_PLAY;
    TickCdAudio();
    CHECK(s_playSteps == 1 && s_fadeSteps == 1);
    g_CdCommandPending = CD_COMMAND_RESUME;
    TickCdAudio();
    CHECK(s_playSteps == 2);
    g_CdCommandPending = CD_COMMAND_PAUSE;
    TickCdAudio();
    CHECK(s_pauseSteps == 1);
    return 0;
}

static int TestEndOfTrackPolicy(void) {
    Reset();
    s_hostEnded = 1;
    g_SceneId = 0x1c;
    TickCdAudio();
    CHECK(g_CdTrackEnded == 1 && g_CdTrackPending == -1);

    Reset();
    s_hostEnded = 1;
    g_CdTrackLoopPoint[0].second = 1;
    g_CdTrackLoopPoint[3].second = 2;
    TickCdAudio();
    CHECK(g_CdTrackPending == 3);
    CHECK(g_CdTrackStep == CD_TRACK_RESTART_WAIT_FOR_DRIVE);
    CHECK(g_CdCommandPending == CD_COMMAND_PLAY);
    CHECK(g_CdCommandStep == CD_PLAY_WAIT_FOR_DRIVE);

    s_trackSteps = 0;
    TickCdAudio();
    CHECK(s_trackSteps == 1);
    CHECK(g_CdTrackStep == CD_TRACK_RESTART_WAIT_FOR_DRIVE);

    Reset();
    s_hostEnded = 1;
    g_CdCurrentTrack = 0xFF;
    TickCdAudio();
    CHECK(g_CdTrackPending == -1 && g_CdCommandPending == CD_COMMAND_NONE);
    CHECK(g_CdTrackEnded == 0);

    Reset();
    s_hostEnded = 1;
    g_CdTrackLoopPoint[0].second = 2;
    g_CdTrackLoopPoint[3].second = 1;
    TickCdAudio();
    CHECK(g_CdTrackPending == -1 && g_CdCommandPending == CD_COMMAND_NONE);
    return 0;
}

int main(void) {
    CHECK(TestInitialization() == 0);
    CHECK(TestRequestDispatch() == 0);
    CHECK(TestEndOfTrackPolicy() == 0);
    puts("CD audio runtime preserves dispatch and end-of-track restart policy");
    return 0;
}
