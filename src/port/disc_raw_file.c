#include "disc_raw_file.h"

#include <limits.h>

#include "disc_iso.h"

int DiscRawFileReadSector(void *context, unsigned int sector,
                          unsigned char *raw) {
    DiscRawFile *disc = context;
    long offset;

    if (disc == NULL || disc->file == NULL || raw == NULL ||
        disc->trackOffset < 0 ||
        (unsigned long)sector > (unsigned long)LONG_MAX ||
        (long)sector >
            (LONG_MAX - disc->trackOffset) / DISC_RAW_SECTOR_SIZE) {
        return 0;
    }
    offset = disc->trackOffset + (long)sector * DISC_RAW_SECTOR_SIZE;
    if (fseek(disc->file, offset, SEEK_SET) != 0) return 0;
    return fread(raw, 1, DISC_RAW_SECTOR_SIZE, disc->file) ==
           DISC_RAW_SECTOR_SIZE;
}
