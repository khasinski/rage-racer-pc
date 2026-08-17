#include "common.h"
#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"

typedef union CdTrackRequestAddress {
    s32 *state;
    u8 *track;
} CdTrackRequestAddress;

void StepCdTrackRequest(void) {
    CdTrackRequestAddress requestAddress;
    s32 state;
    s32 status;
    u8 track;

    switch (g_CdTrackStep) {
    case 0:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdFadeFrames = 0;
        g_CdTrackStep = 1;
        /* fall through */
    case 1:
        if (CdControl(0x16, &g_CdTrackLocs[g_CdTrackPending], 0) == 0) {
            break;
        }
        g_CdTrackStep = 2;
        break;
    case 2:
        state = CdSync(1, 0);
        if (state == 2) {
            g_CdTrackStep = 3;
            break;
        }
        if (state == 5) {
            g_CdTrackStep = 1;
            break;
        }
        break;
    case 3:
        status = g_CdVolume;
        requestAddress.state = &g_CdTrackPending;
        track = (g_CdCurrentTrack = *requestAddress.track);
        g_CdTrackPending = -1;
        g_CdTrackStep = 0;
        SetCdVolume(status);
        break;
    case 4:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdTrackStep = 5;
        /* fall through */
    case 5:
        if (CdControl(0x16, &g_CdTrackLocs[g_CdTrackPending], 0) == 0) {
            break;
        }
        g_CdTrackStep = 6;
        break;
    case 6:
        state = CdSync(1, 0);
        if (state == 2) {
            g_CdTrackStep = 7;
            break;
        }
        if (state == 5) {
            g_CdTrackStep = state;
            break;
        }
        break;
    case 7:
        requestAddress.state = &g_CdTrackPending;
        track = *requestAddress.track;
        g_CdTrackPending = -1;
        g_CdTrackStep = 0;
        g_CdCurrentTrack = track;
        break;
    }
}

void StepCdPlayRequest(void) {
    s32 state;
    s32 status;

    state = g_CdCommandStep;
    switch (state) {
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
        if (status == state) {
            g_CdCommandStep = 3;
            break;
        }
        if (status == 5) {
            g_CdCommandStep = 1;
            break;
        }
        break;
    case 3:
        g_CdCommandPending = CD_COMMAND_NONE;
        g_CdCommandStep = 0;
        break;
    }
}
