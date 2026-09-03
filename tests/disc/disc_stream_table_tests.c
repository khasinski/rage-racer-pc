/* The stream table read off a synthetic disc.
 *
 * The real check is the one against the retail PAL disc, which pins the
 * derivation to a known answer; see stream_table_disc_tests.c. It needs a disc
 * the repository cannot ship, and no NTSC disc can be shipped either. What can be
 * built here is a disc with the shape of the American release: the movies start
 * in different places, the frame counts beside them are twice the PAL ones, and
 * the boot executable is named SLUS. That covers the part of the derivation
 * that has nothing to do with which particular disc it is reading.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_stream_table.h"

#define RAW 2352
#define ISO 2048
#define PVD_SECTOR 16
#define ROOT_SECTOR 17
#define CNF_SECTOR 18
#define EXE_SECTOR 19
#define EXE_SECTORS 4
#define STR_SECTOR 40
/* Four one-sector frames and a sector of sound, eleven times over. */
#define STREAM_SECTORS 5
#define STREAM_FRAMES 4
#define STR_SECTORS (STREAM_SECTORS * RAGE_DISC_STREAM_COUNT)
#define DISC_SECTORS (STR_SECTOR + STR_SECTORS)
#define TABLE_OFFSET 0x40

typedef struct FakeDisc {
    unsigned char sector[DISC_SECTORS][RAW];
} FakeDisc;

static int failures;

static void Check(int condition, const char *what) {
    if (condition) return;
    fprintf(stderr, "FAIL: %s\n", what);
    failures++;
}

static void CheckUnsigned(unsigned int got, unsigned int want,
                          const char *what) {
    if (got == want) return;
    fprintf(stderr, "FAIL: %s: got %u, expected %u\n", what, got, want);
    failures++;
}

static void PutLe16(unsigned char *at, unsigned int value) {
    at[0] = (unsigned char)(value & 0xff);
    at[1] = (unsigned char)((value >> 8) & 0xff);
}

static void PutLe32(unsigned char *at, unsigned int value) {
    PutLe16(at, value & 0xffff);
    PutLe16(at + 2, (value >> 16) & 0xffff);
}

static unsigned char *User(FakeDisc *disc, unsigned int sector) {
    return disc->sector[sector] + 24;
}

/* A Mode 2 Form 1 sector: the subheader says which of the two kinds it is. */
static void MarkSector(FakeDisc *disc, unsigned int sector, int audio) {
    unsigned char *subheader = disc->sector[sector] + 16;
    subheader[0] = 1;
    subheader[1] = 0;
    subheader[2] = (unsigned char)(audio ? 0x64 : 0x08);
    subheader[3] = (unsigned char)(audio ? 0x01 : 0x00);
    memcpy(subheader + 4, subheader, 4);
}

static unsigned int WriteRecord(unsigned char *at, const char *name,
                                unsigned int lba, unsigned int size) {
    unsigned int nameLength = (unsigned int)strlen(name);
    unsigned int length = 33 + nameLength;
    if (length & 1) length++;
    memset(at, 0, length);
    at[0] = (unsigned char)length;
    PutLe32(at + 2, lba);
    PutLe32(at + 10, size);
    at[32] = (unsigned char)nameLength;
    memcpy(at + 33, name, nameLength);
    return length;
}

/* Eleven movies, each numbering its own frames from one, which is what makes
 * the boundaries findable. */
static void WriteStream(FakeDisc *disc) {
    int stream;
    for (stream = 0; stream < RAGE_DISC_STREAM_COUNT; stream++) {
        int index;
        for (index = 0; index < STREAM_SECTORS; index++) {
            unsigned int sector =
                STR_SECTOR + (unsigned int)(stream * STREAM_SECTORS + index);
            unsigned char *body = User(disc, sector);
            if (index >= STREAM_FRAMES) {
                MarkSector(disc, sector, 1);
                continue;
            }
            MarkSector(disc, sector, 0);
            PutLe16(body, 0x0160);      /* magic */
            PutLe16(body + 2, 0x8001);  /* type */
            PutLe16(body + 4, 0);       /* chunk 0 of 1 */
            PutLe16(body + 6, 1);
            PutLe32(body + 8, (unsigned int)index + 1); /* frame number */
            PutLe32(body + 12, 0x7e0);
        }
    }
}

static void WriteExeTable(FakeDisc *disc, const unsigned int *offsets,
                          const unsigned int *frames) {
    unsigned char table[RAGE_DISC_STREAM_COUNT * 8];
    int index;
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
        PutLe32(table + index * 8, offsets[index]);
        PutLe32(table + index * 8 + 4, frames[index]);
    }
    memcpy(User(disc, EXE_SECTOR) + TABLE_OFFSET, table, sizeof(table));
}

static void BuildDisc(FakeDisc *disc, const char *boot, int withTable) {
    static const unsigned int frames[RAGE_DISC_STREAM_COUNT] = {
        2160, 300, 300, 300, 300, 300, 300, 300, 300, 300, 1500};
    unsigned int offsets[RAGE_DISC_STREAM_COUNT];
    unsigned char *root;
    unsigned char *pvd;
    char line[128];
    unsigned int cursor = 0;
    char name[32];
    int index;

    memset(disc, 0, sizeof(*disc));
    for (index = 0; index < DISC_SECTORS; index++) MarkSector(disc, (unsigned)index, 0);

    pvd = User(disc, PVD_SECTOR);
    pvd[0] = 1;
    memcpy(pvd + 1, "CD001", 5);
    pvd[156] = 34;
    PutLe32(pvd + 158, ROOT_SECTOR);
    PutLe32(pvd + 166, ISO);

    root = User(disc, ROOT_SECTOR);
    cursor += WriteRecord(root + cursor, "SYSTEM.CNF;1", CNF_SECTOR, 64);
    snprintf(name, sizeof(name), "%s;1", boot);
    cursor += WriteRecord(root + cursor, name, EXE_SECTOR, EXE_SECTORS * ISO);
    cursor += WriteRecord(root + cursor, "RAGE.BIN;1", 30, ISO);
    WriteRecord(root + cursor, "RAGE.STR;1", STR_SECTOR, STR_SECTORS * ISO);

    snprintf(line, sizeof(line), "BOOT = cdrom:\\%s;1\r\nTCB = 4\r\n", boot);
    memcpy(User(disc, CNF_SECTOR), line, strlen(line));

    WriteStream(disc);
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++)
        offsets[index] = (unsigned int)(index * STREAM_SECTORS);
    if (withTable) WriteExeTable(disc, offsets, frames);
}

static int ReadFake(void *context, unsigned int sector, unsigned char *raw) {
    FakeDisc *disc = context;
    if (sector >= DISC_SECTORS) return 0;
    memcpy(raw, disc->sector[sector], RAW);
    return 1;
}

static void ExpectTable(const DiscIdentity *identity) {
    static const unsigned int frames[RAGE_DISC_STREAM_COUNT] = {
        2160, 300, 300, 300, 300, 300, 300, 300, 300, 300, 1500};
    int index;
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
        CheckUnsigned(identity->table.offset[index],
                      (unsigned int)(index * STREAM_SECTORS), "stream offset");
        CheckUnsigned(identity->table.frames[index], frames[index],
                      "stream frame count");
        CheckUnsigned(identity->table.span[index], STREAM_SECTORS,
                      "stream sector span");
    }
}

static void TestAmericanDisc(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    BuildDisc(disc, "SLUS_004.03", 1);
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(strcmp(identity.boot, "SLUS_004.03") == 0, "reads BOOT from SYSTEM.CNF");
    Check(strcmp(identity.region, "NTSC-U") == 0, "calls SLUS NTSC-U");
    Check(identity.tableValid, "reads the stream table");
    ExpectTable(&identity);
    free(disc);
}

/* The same disc with no SYSTEM.CNF: the executable in the root directory is
 * the same serial by another route. */
static void TestBootNameFallback(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    BuildDisc(disc, "SLUS_004.03", 1);
    memset(User(disc, CNF_SECTOR), 0, ISO);
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(strcmp(identity.boot, "SLUS_004.03") == 0,
          "falls back to the executable's name");
    Check(identity.tableValid, "still reads the stream table");
    free(disc);
}

static void TestEuropeanName(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    BuildDisc(disc, "SCES_006.50", 1);
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(strcmp(identity.region, "PAL") == 0, "calls SCES PAL");
    Check(identity.tableValid, "reads the stream table");
    free(disc);
}

/* Nothing in the executable matches where the movies really are, so the port
 * has to say so and keep what it was built with. */
static void TestMissingTable(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    BuildDisc(disc, "SLUS_004.03", 0);
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(!identity.tableValid, "refuses a table it could not find");
    Check(identity.reason != NULL, "says why");
    free(disc);
}

static void TestInvalidFrameTableIsNotPartiallyPublished(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    unsigned char *table;
    int index;

    BuildDisc(disc, "SLUS_004.03", 1);
    table = User(disc, EXE_SECTOR) + TABLE_OFFSET;
    PutLe32(table + 5 * 8 + 4, 0);
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(!identity.tableValid, "rejects a zero movie frame count");
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
        CheckUnsigned(identity.table.frames[index], 0,
                      "does not publish a partial frame table");
    }
    free(disc);
}

/* A movie boundary found one sector late stops matching the executable, which
 * is what keeps a half-right scan from being adopted. */
static void TestShiftedBoundary(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    unsigned int sector = STR_SECTOR + STREAM_SECTORS;
    BuildDisc(disc, "SLUS_004.03", 1);
    /* Hide the first frame of the second movie, so its start is read as the
     * sector after the one the executable names. */
    memset(User(disc, sector), 0, 16);
    MarkSector(disc, sector, 1);
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(!identity.tableValid, "refuses a scan the executable disagrees with");
    Check(identity.reason != NULL, "says why");
    free(disc);
}

static void TestPartialStrMagic(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    int stream;

    BuildDisc(disc, "SLUS_004.03", 1);
    for (stream = 0; stream < RAGE_DISC_STREAM_COUNT; stream++) {
        unsigned int sector =
            STR_SECTOR + (unsigned int)(stream * STREAM_SECTORS + 4);
        unsigned char *body = User(disc, sector);

        MarkSector(disc, sector, 0);
        PutLe16(body, 0x0160);
        PutLe16(body + 2, 0);
        PutLe16(body + 4, 0);
        PutLe32(body + 8, 0);
    }
    Check(DiscIdentify(ReadFake, disc, &identity), "identifies the disc");
    Check(identity.tableValid, "ignores sectors with only half the STR magic");
    free(disc);
}

static void TestMalformedDirectoryRecord(void) {
    FakeDisc *disc = malloc(sizeof(*disc));
    DiscIdentity identity;
    unsigned char *record;

    BuildDisc(disc, "SLUS_004.03", 1);
    record = User(disc, ROOT_SECTOR);
    record[32] = record[0];
    Check(DiscIdentify(ReadFake, disc, &identity),
          "recognizes a readable disc with a malformed directory");
    Check(identity.boot[0] == '\0' && !identity.tableValid,
          "rejects a file name that extends beyond its directory record");
    free(disc);
}

static void TestUnreadableDisc(void) {
    DiscIdentity identity;
    Check(!DiscIdentify(ReadFake, NULL, NULL), "rejects a null result");
    Check(!DiscIdentify(NULL, NULL, &identity), "rejects a disc it cannot read");
    Check(strcmp(identity.region, "unknown") == 0, "reports no region");
}

static void TestRegionNames(void) {
    Check(strcmp(DiscRegionForBootName("SLPS_009.00"), "NTSC-J") == 0,
          "calls SLPS NTSC-J");
    Check(strcmp(DiscRegionForBootName("SLES_012.34"), "PAL") == 0,
          "calls SLES PAL");
    Check(strcmp(DiscRegionForBootName("PSX.EXE"), "unknown") == 0,
          "admits it does not know");
}

int main(void) {
    TestAmericanDisc();
    TestBootNameFallback();
    TestEuropeanName();
    TestMissingTable();
    TestInvalidFrameTableIsNotPartiallyPublished();
    TestShiftedBoundary();
    TestPartialStrMagic();
    TestMalformedDirectoryRecord();
    TestUnreadableDisc();
    TestRegionNames();
    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("the stream table comes off the disc that is mounted\n");
    return 0;
}
