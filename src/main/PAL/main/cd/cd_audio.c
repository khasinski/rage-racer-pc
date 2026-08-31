#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "game/track_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "psyq/snd.h"

long HostCdAudioEnded(void);


void StepCdPauseRequest(void) {
    s32 state;
    s32 result;
    s32 currentTime;
    s32 bestTime;
    s32 enteredTime;

    state = g_CdCommandStep;

    switch (state) {
    case 0:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdCommandStep = 1;
        /* fallthrough */

    case 1:
        if (CdControl(0x11, 0, g_CdLocResult) != 0) {
            g_CdCommandStep = 2;
        }
        break;

    case 2:
        result = CdSync(1, 0);
        if (result == 2) {
            g_CdCommandStep = 3;
        } else if (result == 5) {
            g_CdCommandStep = 1;
        }
        break;

    case 3:
        g_CdTrackElapsedLoc.minute = g_CdLocResult[2];
        g_CdTrackElapsedLoc.sector = 0;
        g_CdTrackElapsedLoc.second = g_CdLocResult[3];

        currentTime = CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
        bestTime = CdPosToInt_Local(&g_CdTrackLoopPoint[0]);
        if (bestTime < currentTime) {
            enteredTime = CdPosToInt_Local(&g_CdTrackElapsedLoc);
            currentTime = CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
            if (enteredTime >= currentTime) {
                g_CdRestartOnResume = 1;
            } else {
                g_CdRestartOnResume = 0;
            }
        } else {
            g_CdRestartOnResume = 0;
        }

        g_CdCommandStep = 4;
        /* fallthrough */

    case 4:
        if (CdControl(9, 0, 0) != 0) {
            g_CdCommandStep = 5;
        }
        break;

    case 5:
        result = CdSync(1, 0);
        if (result == 2) {
            g_CdCommandStep = 6;
        } else if (result == 5) {
            g_CdCommandStep = 4;
        }
        break;

    case 6:
        g_CdCommandPending = CD_COMMAND_NONE;
        g_CdCommandStep = 0;
        break;
    }
}
void StepCdResumeRequest(void) {
    s32 status;

    switch (g_CdCommandStep) {
    case 0:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdCommandStep = 1;
        /* fall through */
    case 1:
        if (CdControl(3, 0, 0) == 0) {
            break;
        }
        g_CdCommandStep = 2;
        break;
    case 2:
        status = CdSync(1, 0);
        if (status == 2) {
            g_CdCommandStep = 3;
        } else if (status == 5) {
            g_CdCommandStep = 1;
        }
        break;
    case 3:
        g_CdCommandPending = CD_COMMAND_NONE;
        g_CdCommandStep = 0;
        break;
    }
}

void InitCdAudio(void) {
    u8 *status;

    SsSetSpuInputAttr(0, 0, 1);
    SsSetSerialVol(0, 0x7FFF, 0x7FFF);
    status = &g_CdModeParam;
    *status = 7;
    CdControl(0xE, status, 0);
    BuildCdTrackTable();

    g_CdTrackPending = -1;
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdCurrentTrack = 2;
    g_CdTrackStep = 0;
    g_CdCommandStep = 0;
    g_CdMixPreset = 0;
    g_CdRestartOnResume = 0;
    g_CdVolume = 0x7F;
    g_CdFadeFrames = 0;
    SetCdVolume(0x7F);
}

void TickCdAudio(void) {
    s32 temp;
    s32 value;

    if (g_CdTrackPending < 0) {
        switch (g_CdCommandPending) {
        case CD_COMMAND_NONE:
            break;
        case CD_COMMAND_PLAY:
            StepCdPlayRequest();
            break;
        case CD_COMMAND_PAUSE:
            StepCdPauseRequest();
            break;
        case CD_COMMAND_RESUME:
            StepCdResumeRequest();
            break;
        }
    } else {
        StepCdTrackRequest();
    }

    if (HostCdAudioEnded()) {
        if (g_SceneId == 0x1C) {
            g_CdTrackEnded = 1;
        } else {
            temp = CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
            value = CdPosToInt_Local(&g_CdTrackLoopPoint[0]);
            if (value < temp) {
                g_CdTrackStep = 4;
                g_CdCommandPending = CD_COMMAND_PLAY;
                g_CdCommandStep = 0;
                g_CdTrackPending = g_CdCurrentTrack;
            }
        }
    }

    StepCdVolumeFade();
}


void SelectTrackCameraTable(void *block, s32 variant) {
    TrackCameraTable *table = block;
    s32 offset;

    if (variant != 0) {
        if (g_GrandPrixSeries != 0) {
            offset = table->seriesOffset[1];
        } else {
            offset = table->seriesOffset[0];
        }
    } else {
        offset = table->defaultOffset;
    }

    g_TrackCameras = ResolveTrackCameraOffset(table, offset);
}
