#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>

CdlLOC g_CdTrackLocs[18];
static CdlLOC s_bgmTracks[16];
CdlLOC *g_CdBgmTrackLocs = s_bgmTracks;
CdlFILE g_CdSearchFile;
s32 g_CdTocEntryCount;
char *g_CdAudioFileNames[16];

static s32 s_searchCount;

long CdGetToc(CdlLOC *tracks) {
    tracks[1].sector = 10;
    tracks[2].sector = 20;
    return 2;
}

long CdPosToInt_Local(CdlLOC *location) {
    return location->sector;
}

CdlLOC *CdIntToPos(long sectors, CdlLOC *location) {
    location->sector = (u8)sectors;
    return location;
}

CdlFILE *DsSearchFile(CdlFILE *file, char *name) {
    (void)name;
    if (s_searchCount == 3) {
        return NULL;
    }
    file->pos.minute = (u8)(s_searchCount + 1);
    s_searchCount++;
    return file;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    s32 index;

    for (index = 0; index < 16; index++) {
        g_CdAudioFileNames[index] = "track";
    }

    BuildCdTrackTable();
    CHECK(g_CdTrackLocs[1].sector == 70);
    CHECK(g_CdTrackLocs[2].sector == 80);
    CHECK(s_searchCount == 3);
    CHECK(s_bgmTracks[0].minute == 1);
    CHECK(s_bgmTracks[1].minute == 2);
    CHECK(s_bgmTracks[2].minute == 3);
    CHECK(s_bgmTracks[3].minute == 0);
    CHECK(g_CdTocEntryCount == 16);

    puts("CD track table tests passed");
    return 0;
}
