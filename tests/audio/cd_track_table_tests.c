#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

#include <stdio.h>
#include <string.h>

CdlLOC g_CdTrackLocs[CD_TRACK_LOCATION_COUNT];
static CdlLOC s_bgmTracks[CD_FILE_TRACK_COUNT];
CdlLOC *g_CdBgmTrackLocs = s_bgmTracks;
CdlFILE g_CdSearchFile;
char *g_CdAudioFileNames[CD_FILE_TRACK_COUNT];

static s32 s_searchCount;
static s32 s_tocCount = 2;

long CdGetToc(CdlLOC *tracks) {
    s32 index;

    for (index = 1; index <= s_tocCount; index++) {
        tracks[index].sector = (u8)(index * 10);
    }
    return s_tocCount;
}

int CdPosToInt(CdlLOC *location) {
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

    for (index = 0; index < CD_FILE_TRACK_COUNT; index++) {
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

    memset(g_CdTrackLocs, 0, sizeof(g_CdTrackLocs));
    s_searchCount = 0;
    s_tocCount = CD_TOC_CAPACITY - 1;
    BuildCdTrackTable();
    CHECK(g_CdTrackLocs[CD_TRACK_LOCATION_COUNT - 1].sector ==
          (u8)(170 + 60));

    puts("CD track table tests passed");
    return 0;
}
