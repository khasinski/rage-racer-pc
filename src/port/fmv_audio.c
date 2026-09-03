#include "fmv_audio.h"

#include <stdio.h>

#include "game/cd.h"
#include "host_disc.h"
#include "psyq/cd.h"
#include <psyz/cd.h>
#include "runtime_config.h"

enum {
    FMV_SECTOR_SIZE = 2352,
    XA_FILTER_SEARCH_SECTORS = 16,
    XA_SUBMODE_AUDIO = 0x04,
    XA_SUBMODE_MASK = 0x0E,
    XA_SUBHEADER_FILE = 0x10,
    XA_SUBHEADER_CHANNEL = 0x11,
    XA_SUBHEADER_SUBMODE = 0x12,
    RAGE_CDL_SETLOC = 0x02,
    RAGE_CDL_READN = 0x06,
    RAGE_CDL_PAUSE = 0x09,
    RAGE_CDL_SETFILTER = 0x0D,
    RAGE_CDL_SETMODE = 0x0E,
    RAGE_CDL_MODE_DA = 0x01,
    RAGE_CDL_MODE_RT = 0x40,
};

static int s_xaPlaying;
static int s_xaTailAllowed;

static void FinishXaAudio(void) {
    unsigned char mode = RAGE_CDL_MODE_DA | CdlModeSpeed;

    Psyz_CdSetXaEndSector(-1);
    CdControl(RAGE_CDL_SETMODE, &mode, NULL);
    s_xaPlaying = 0;
    s_xaTailAllowed = 0;
}

static void StopXaAudio(void) {
    if (s_xaPlaying) {
        CdControl(RAGE_CDL_PAUSE, NULL, NULL);
    }
    FinishXaAudio();
}

static void PrepareXaAudioStart(void) {
    if (s_xaPlaying) {
        CdControl(RAGE_CDL_PAUSE, NULL, NULL);
    }
    Psyz_CdSetXaEndSector(-1);
    s_xaPlaying = 0;
    s_xaTailAllowed = 0;
}

void HostFmvAudioTick(void) {
    if (s_xaPlaying && !Psyz_CdAudioPlaying()) {
        if (RuntimeConfigEnabled("diagnostics.fmv_trace")) {
            fprintf(stderr, "fmv xa end\n");
        }
        FinishXaAudio();
    }
}

int FmvXaStreaming(void) {
    HostFmvAudioTick();
    return s_xaPlaying;
}

void HostFmvAudioStart(unsigned int firstSector, unsigned int sectorCount) {
    unsigned char raw[FMV_SECTOR_SIZE];
    unsigned char filter[2] = {0, 0};
    unsigned char mode = RAGE_CDL_MODE_RT | CdlModeSpeed;
    CdlLOC location;
    int absoluteFirst;
    int absoluteEnd;
    unsigned int index;
    unsigned int searchCount = sectorCount < XA_FILTER_SEARCH_SECTORS
                                   ? sectorCount
                                   : XA_FILTER_SEARCH_SECTORS;

    PrepareXaAudioStart();
    if (!HostStreamAbsoluteRange(firstSector, sectorCount, &absoluteFirst,
                                 &absoluteEnd)) {
        FinishXaAudio();
        return;
    }
    for (index = 0; index < searchCount; index++) {
        if (!HostReadStreamSector(firstSector + index, raw)) {
            FinishXaAudio();
            return;
        }
        if ((raw[XA_SUBHEADER_SUBMODE] & XA_SUBMODE_MASK) ==
            XA_SUBMODE_AUDIO) {
            filter[0] = raw[XA_SUBHEADER_FILE];
            filter[1] = raw[XA_SUBHEADER_CHANNEL];
            break;
        }
    }

    if (index == searchCount) {
        FinishXaAudio();
        return;
    }

    CdIntToPos(absoluteFirst, &location);
    if (RuntimeConfigEnabled("diagnostics.fmv_trace")) {
        fprintf(stderr, "fmv xa start: sector=%d filter=%u/%u\n",
                absoluteFirst,
                filter[0], filter[1]);
    }
    CdControl(RAGE_CDL_SETFILTER, filter, NULL);
    CdControl(RAGE_CDL_SETMODE, &mode, NULL);
    CdControl(RAGE_CDL_SETLOC, (unsigned char *)&location, NULL);
    Psyz_CdSetXaEndSector(absoluteEnd);
    CdControl(RAGE_CDL_READN, NULL, NULL);
    s_xaPlaying = Psyz_CdAudioPlaying();
    if (!s_xaPlaying) {
        FinishXaAudio();
    }
}

void HostFmvAudioAllowTail(void) {
    s_xaTailAllowed = 1;
}

void HostFmvAudioEnd(void) {
    if (s_xaPlaying && !s_xaTailAllowed) {
        StopXaAudio();
    } else if (s_xaPlaying &&
               RuntimeConfigEnabled("diagnostics.fmv_trace")) {
        fprintf(stderr, "fmv video end: xa tail continues\n");
    }
}
