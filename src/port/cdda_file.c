#include <string.h>

#include "psyq/cd.h"

int Psyz_CdGetTrackSector(int track);

static int ParseTrackNumber(const char *name, int *track,
                            const char **matchedName) {
    const char *marker = name;

    while ((marker = strstr(marker, "DA")) != NULL) {
        if (marker[2] >= '0' && marker[2] <= '9' &&
            marker[3] >= '0' && marker[3] <= '9') {
            *track = (marker[2] - '0') * 10 + marker[3] - '0';
            *matchedName = marker;
            return 1;
        }
        marker += 2;
    }
    return 0;
}

CdlFILE *DsSearchFile(CdlFILE *file, char *name) {
    const char *displayName;
    int track;
    int sector;

    if (file == NULL || name == NULL ||
        !ParseTrackNumber(name, &track, &displayName)) {
        return NULL;
    }
    sector = Psyz_CdGetTrackSector(track);
    if (sector < 0) return NULL;
    CdIntToPos(sector, &file->pos);
    file->size = 0;
    strncpy(file->name, displayName, sizeof(file->name) - 1);
    file->name[sizeof(file->name) - 1] = '\0';
    return file;
}
