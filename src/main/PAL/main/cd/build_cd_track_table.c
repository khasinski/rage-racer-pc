#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#define CD_TRACK_LEAD_IN_SECTORS 0x3C
#define CD_FILE_TRACK_COUNT 0x10

void BuildCdTrackTable(void) {
    s32 audioTrackCount = CdGetToc(g_CdTrackLocs);
    s32 index;

    if (audioTrackCount > 0) {
        for (index = 1; index <= audioTrackCount; index++) {
            CdlLOC *track = &g_CdTrackLocs[index];
            CdIntToPos(CdPosToInt_Local(track) + CD_TRACK_LEAD_IN_SECTORS,
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
