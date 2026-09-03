#include "disc_iso.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TEST_SECTOR_COUNT = 20,
    ROOT_SECTOR = 17,
    FILE_SECTOR = 18,
};

typedef struct FakeDisc {
    unsigned char sectors[TEST_SECTOR_COUNT][DISC_RAW_SECTOR_SIZE];
} FakeDisc;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void WriteLe32(unsigned char *data, unsigned int value) {
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
    data[2] = (unsigned char)(value >> 16);
    data[3] = (unsigned char)(value >> 24);
}

static int ReadFake(void *context, unsigned int sector, unsigned char *raw) {
    FakeDisc *disc = context;

    if (sector >= TEST_SECTOR_COUNT) return 0;
    memcpy(raw, disc->sectors[sector], DISC_RAW_SECTOR_SIZE);
    return 1;
}

static void BuildDisc(FakeDisc *disc, int userOffset) {
    static const char name[] = "RAGE.BIN;1";
    unsigned char *volume;
    unsigned char *record;
    unsigned int recordLength = 33 + sizeof(name) - 1;

    memset(disc, 0, sizeof(*disc));
    if (recordLength & 1) recordLength++;
    volume = disc->sectors[16] + userOffset;
    volume[0] = 1;
    memcpy(volume + 1, "CD001", 5);
    volume[156] = 34;
    WriteLe32(volume + 158, ROOT_SECTOR);
    WriteLe32(volume + 166, DISC_ISO_SECTOR_SIZE);

    record = disc->sectors[ROOT_SECTOR] + userOffset;
    record[0] = (unsigned char)recordLength;
    WriteLe32(record + 2, FILE_SECTOR);
    WriteLe32(record + 10, 3000);
    record[32] = (unsigned char)(sizeof(name) - 1);
    memcpy(record + 33, name, sizeof(name) - 1);
    memset(disc->sectors[FILE_SECTOR] + userOffset, 0x11,
           DISC_ISO_SECTOR_SIZE);
    memset(disc->sectors[FILE_SECTOR + 1] + userOffset, 0x22,
           DISC_ISO_SECTOR_SIZE);
}

static int TestDiscAtOffset(int userOffset) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIsoReader reader;
    DiscIsoFile file;
    unsigned char *data;

    CHECK(disc != NULL);
    BuildDisc(disc, userOffset);
    CHECK(DiscIsoOpen(&reader, ReadFake, disc));
    CHECK(reader.userOffset == userOffset);
    CHECK(DiscIsoFindFile(&reader, "rage.bin", &file));
    CHECK(file.lba == FILE_SECTOR && file.size == 3000);
    data = DiscIsoReadWholeFile(&reader, &file);
    CHECK(data != NULL);
    CHECK(data[0] == 0x11 && data[2047] == 0x11);
    CHECK(data[2048] == 0x22 && data[2999] == 0x22);
    free(data);
    free(disc);
    return 0;
}

static int TestMalformedDirectory(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIsoReader reader;
    DiscIsoFile file;
    unsigned char *record;

    CHECK(disc != NULL);
    BuildDisc(disc, DISC_MODE2_USER_OFFSET);
    record = disc->sectors[ROOT_SECTOR] + DISC_MODE2_USER_OFFSET;
    record[32] = record[0];
    CHECK(DiscIsoOpen(&reader, ReadFake, disc));
    CHECK(!DiscIsoFindFile(&reader, "RAGE.BIN", &file));
    free(disc);
    return 0;
}

static int TestDirectorySizeBounds(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIsoReader reader;
    DiscIsoFile file;
    unsigned char *volume;
    unsigned char *record;

    CHECK(disc != NULL);
    BuildDisc(disc, DISC_MODE2_USER_OFFSET);
    volume = disc->sectors[16] + DISC_MODE2_USER_OFFSET;
    record = disc->sectors[ROOT_SECTOR] + DISC_MODE2_USER_OFFSET;
    WriteLe32(volume + 166, record[0] - 1);
    CHECK(DiscIsoOpen(&reader, ReadFake, disc));
    CHECK(!DiscIsoFindFile(&reader, "RAGE.BIN", &file));
    free(disc);
    return 0;
}

static int TestArguments(void) {
    DiscIsoReader reader;
    DiscIsoFile file = {1, 1};
    unsigned char user[DISC_ISO_SECTOR_SIZE];

    CHECK(!DiscIsoOpen(NULL, ReadFake, NULL));
    CHECK(!DiscIsoOpen(&reader, NULL, NULL));
    memset(&reader, 0, sizeof(reader));
    CHECK(!DiscIsoReadUserSector(&reader, 0, user));
    CHECK(!DiscIsoFindFile(&reader, NULL, &file));
    CHECK(DiscIsoReadWholeFile(&reader, NULL) == NULL);
    return 0;
}

int main(void) {
    CHECK(TestDiscAtOffset(16) == 0);
    CHECK(TestDiscAtOffset(DISC_MODE2_USER_OFFSET) == 0);
    CHECK(TestMalformedDirectory() == 0);
    CHECK(TestDirectorySizeBounds() == 0);
    CHECK(TestArguments() == 0);
    puts("ISO readers share validated MODE1 and MODE2 directory traversal");
    return 0;
}
