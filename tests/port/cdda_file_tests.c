#include "psyq/cd.h"

#include <stdio.h>
#include <string.h>

static int s_requestedTrack;
static int s_trackSector;

int Psyz_CdGetTrackSector(int track) {
    s_requestedTrack = track;
    return s_trackSector;
}

CdlLOC *CdIntToPos(long sector, CdlLOC *position) {
    position->minute = (unsigned char)(sector / (60 * 75));
    position->second = (unsigned char)((sector / 75) % 60);
    position->sector = (unsigned char)(sector % 75);
    return position;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int TestTrackLookup(void) {
    CdlFILE file;
    char name[] = "\\CDDA\\DA12RPLY.DA;1";

    memset(&file, 0x7f, sizeof(file));
    s_requestedTrack = -1;
    s_trackSector = 5 * 60 * 75 + 4 * 75 + 3;
    CHECK(DsSearchFile(&file, name) == &file);
    CHECK(s_requestedTrack == 12);
    CHECK(file.pos.minute == 5 && file.pos.second == 4 &&
          file.pos.sector == 3);
    CHECK(file.size == 0);
    CHECK(strcmp(file.name, "DA12RPLY.DA;1") == 0);
    return 0;
}

static int TestInvalidLookup(void) {
    CdlFILE file;
    char invalid[] = "\\CDDA\\TRACK.DA;1";
    char missing[] = "DA03TECH.DA;1";

    CHECK(DsSearchFile(NULL, invalid) == NULL);
    CHECK(DsSearchFile(&file, NULL) == NULL);
    CHECK(DsSearchFile(&file, invalid) == NULL);
    s_trackSector = -1;
    CHECK(DsSearchFile(&file, missing) == NULL);
    CHECK(s_requestedTrack == 3);
    return 0;
}

int main(void) {
    CHECK(TestTrackLookup() == 0);
    CHECK(TestInvalidLookup() == 0);
    puts("CD-DA pseudo-files resolve validated track numbers");
    return 0;
}
