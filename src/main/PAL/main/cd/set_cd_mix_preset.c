#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"


void SetCdMixPreset(s32 preset) {
    g_CdMixPreset = preset;
    SetCdVolume(g_CdVolume);
}


void BuildCdTrackTable(void) {
    s32 i;
    CdlLOC *tocDst;
    CdlFILE *file;
    char **fileName;

    g_CdTocEntryCount = CdGetToc(g_CdTrackLocs);
    if (g_CdTocEntryCount > 0) {
        for (i = 1; i <= g_CdTocEntryCount; i++) {
            CdlLOC *entry = &g_CdTrackLocs[i];

            CdIntToPos(CdPosToInt_Local(entry) + 0x3C, entry);
        }
    }

    file = &g_CdSearchFile;
    tocDst = g_CdBgmTrackLocs;
    fileName = g_CdAudioFileNames;
    for (i = 0; i < 0x10; i++) {
        if (DsSearchFile(file, *fileName) == 0) {
            break;
        }
        *tocDst++ = file->pos;
        fileName++;
    }

    g_CdTocEntryCount = 0x10;
}
