#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"


void SetCdMixPreset(s32 preset) {
    g_CdMixPreset = preset;
    SetCdVolume(g_CdVolume);
}


void BuildCdTrackTable(void) {
    CdlLOC *toc;
    s32 i;
    CdlLOC *tocDst;
    CdlFILE *file;
    char **fileName;
    s32 count;

    toc = g_CdTrackLocs;
    g_CdTocEntryCount = CdGetToc(toc);
    if (g_CdTocEntryCount > 0) {
        i = 1;
        toc++;
        do {
            CdIntToPos(CdPosToInt_Local(toc) + 0x3C, toc);
            count = g_CdTocEntryCount;
            i++;
        } while ((count >= i) ? (toc++, 1) : (toc++, 0));
    }

    i = 2;
    file = &g_CdSearchFile;
    tocDst = g_CdBgmTrackLocs;
    fileName = g_CdAudioFileNames;
    do {
        if (DsSearchFile(file, *fileName) == 0) {
            break;
        }
        *tocDst = file->pos;
        tocDst++;

        i++;
        fileName++;
    } while (i < 0x12);

    g_CdTocEntryCount = 0x10;
}
