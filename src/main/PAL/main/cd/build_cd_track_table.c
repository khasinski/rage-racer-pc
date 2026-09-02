#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#define CD_TRACK_LEAD_IN_SECTORS 0x3C

void BuildCdTrackTable(void) {
    CdlLOC toc[CD_TOC_CAPACITY];
    s32 audioTrackCount = CdGetToc(toc);
    s32 copiedTrackCount = audioTrackCount;
    s32 index;

    if (copiedTrackCount >= CD_TRACK_LOCATION_COUNT) {
        copiedTrackCount = CD_TRACK_LOCATION_COUNT - 1;
    }
    if (copiedTrackCount > 0) {
        for (index = 1; index <= copiedTrackCount; index++) {
            CdlLOC *track = &g_CdTrackLocs[index];
            CdIntToPos(CdPosToInt_Local(&toc[index]) +
                           CD_TRACK_LEAD_IN_SECTORS,
                       track);
        }
    }

    for (index = 0; index < CD_FILE_TRACK_COUNT; index++) {
        if (DsSearchFile(&g_CdSearchFile, g_CdAudioFileNames[index]) == NULL) {
            break;
        }
        g_CdBgmTrackLocs[index] = g_CdSearchFile.pos;
    }

    g_CdTocEntryCount = CD_FILE_TRACK_COUNT;
}
