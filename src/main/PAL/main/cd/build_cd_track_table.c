#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <string.h>

enum { CD_TRACK_LEAD_IN_SECTORS = 60 };

static void CopyTocTrackLocations(CdlLOC *toc, s32 trackCount) {
    s32 index;

    memset(g_CdTrackLocs, 0, sizeof(g_CdTrackLocs));
    if (trackCount <= 0) {
        return;
    }
    if (trackCount >= CD_TRACK_LOCATION_COUNT) {
        trackCount = CD_TRACK_LOCATION_COUNT - 1;
    }

    for (index = 1; index <= trackCount; index++) {
        CdIntToPos(CdPosToInt(&toc[index]) + CD_TRACK_LEAD_IN_SECTORS,
                   &g_CdTrackLocs[index]);
    }
}

static void FindFileBackedTrackLocations(void) {
    s32 index;

    memset(g_CdBgmTrackLocs, 0,
           sizeof(*g_CdBgmTrackLocs) * CD_FILE_TRACK_COUNT);
    for (index = 0; index < CD_FILE_TRACK_COUNT; index++) {
        if (DsSearchFile(&g_CdSearchFile, g_CdAudioFileNames[index]) == NULL) {
            break;
        }
        g_CdBgmTrackLocs[index] = g_CdSearchFile.pos;
    }
}

void BuildCdTrackTable(void) {
    CdlLOC toc[CD_TOC_CAPACITY];

    CopyTocTrackLocations(toc, CdGetToc(toc));
    FindFileBackedTrackLocations();
}
