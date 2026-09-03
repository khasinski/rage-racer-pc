#include "disc_raw_file.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "disc_iso.h"

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int TestSectorRead(void) {
    unsigned char sector[DISC_RAW_SECTOR_SIZE];
    unsigned char output[DISC_RAW_SECTOR_SIZE];
    DiscRawFile disc;
    FILE *file = tmpfile();

    CHECK(file != NULL);
    memset(sector, 0x11, sizeof(sector));
    CHECK(fwrite("offset", 1, 6, file) == 6);
    CHECK(fwrite(sector, 1, sizeof(sector), file) == sizeof(sector));
    memset(sector, 0x22, sizeof(sector));
    CHECK(fwrite(sector, 1, sizeof(sector), file) == sizeof(sector));
    disc.file = file;
    disc.trackOffset = 6;
    CHECK(DiscRawFileReadSector(&disc, 1, output));
    CHECK(output[0] == 0x22 && output[sizeof(output) - 1] == 0x22);
    fclose(file);
    return 0;
}

static int TestInvalidReads(void) {
    unsigned char output[DISC_RAW_SECTOR_SIZE];
    DiscRawFile disc = {NULL, 0};

    CHECK(!DiscRawFileReadSector(NULL, 0, output));
    CHECK(!DiscRawFileReadSector(&disc, 0, output));
    disc.file = tmpfile();
    CHECK(disc.file != NULL);
    CHECK(!DiscRawFileReadSector(&disc, 0, NULL));
    disc.trackOffset = -1;
    CHECK(!DiscRawFileReadSector(&disc, 0, output));
    disc.trackOffset = LONG_MAX;
    CHECK(!DiscRawFileReadSector(&disc, 1, output));
    fclose(disc.file);
    return 0;
}

int main(void) {
    CHECK(TestSectorRead() == 0);
    CHECK(TestInvalidReads() == 0);
    puts("raw disc files enforce track offsets and seek bounds");
    return 0;
}
