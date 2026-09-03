/* The stream table derived from a real disc.
 *
 * This is the check with force behind it, because the answer is known in
 * advance for one disc: retail's own PAL table is compiled into the port as
 * g_RetailPalStreamTable, and the table derived from the PAL disc has to equal
 * it entry for entry, all eleven sector offsets and all eleven frame counts,
 * with the spans that follow from them. Arriving at the same twenty-two
 * numbers a second way, by walking RAGE.STR and then finding those offsets in
 * the disc's own boot executable, is not a coincidence a broken derivation can
 * produce.
 *
 * It drives the port's own derivation over a disc image directly, so what is
 * under test is the code the game runs and not a printout of it.
 *
 * The repository cannot ship a disc, so this skips with 77 when there is none.
 * It reads the same places the rest of the disc tests do, and checks a second
 * disc when RAGE_PORT_NTSC_CUE names one.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_cue.h"
#include "disc_stream_table.h"

/* ctest reads this exit code as "skipped" rather than "failed". */
#define SKIP_EXIT_CODE 77
#define RAW_SECTOR_SIZE 2352
#define DEFAULT_CUE "disc/PAL/Rage Racer (Europe).cue"

/* The American pressing, for when it is the disc being read. */
static const unsigned int NTSC_U_OFFSETS[RAGE_DISC_STREAM_COUNT] = {
    0x0000, 0x2F17, 0x3540, 0x3B69, 0x4192, 0x47BB,
    0x4DE4, 0x540D, 0x5A36, 0x605F, 0x6688};
static const unsigned int NTSC_U_FRAMES[RAGE_DISC_STREAM_COUNT] = {
    2160, 300, 300, 300, 300, 300, 300, 300, 300, 300, 1500};

static int failures;

typedef struct ImageReader {
    FILE *file;
    long firstSector;
} ImageReader;

static void Fail(const char *disc, const char *what) {
    fprintf(stderr, "FAIL: %s: %s\n", disc, what);
    failures++;
}

static void CheckUnsigned(const char *disc, const char *what, int index,
                              unsigned int got, unsigned int want) {
    if (got == want) return;
    fprintf(stderr, "FAIL: %s: %s %d is %u (0x%X), expected %u (0x%X)\n",
            disc, what, index, got, got, want, want);
    failures++;
}

static int ReadImageSector(void *context, unsigned int sector,
                               unsigned char *raw) {
    ImageReader *reader = context;
    long offset = (reader->firstSector + (long)sector) * RAW_SECTOR_SIZE;
    if (fseek(reader->file, offset, SEEK_SET) != 0) return 0;
    return fread(raw, 1, RAW_SECTOR_SIZE, reader->file) == RAW_SECTOR_SIZE;
}

static int EndsWith(const char *path, const char *suffix) {
    size_t pathLength = strlen(path);
    size_t suffixLength = strlen(suffix);
    size_t index;
    if (pathLength < suffixLength) return 0;
    for (index = 0; index < suffixLength; index++) {
        if (tolower((unsigned char)path[pathLength - suffixLength + index]) !=
            tolower((unsigned char)suffix[index]))
            return 0;
    }
    return 1;
}

static int OpenDisc(const char *path, ImageReader *reader) {
    char image[2048];
    if (EndsWith(path, ".cue")) {
        long trackOffset;
        if (!DiscCueResolveDataTrack(path, image, sizeof(image), &trackOffset))
            return 0;
        reader->firstSector = trackOffset / RAW_SECTOR_SIZE;
    } else {
        snprintf(image, sizeof(image), "%s", path);
        reader->firstSector = 0;
    }
    reader->file = fopen(image, "rb");
    return reader->file != NULL;
}

/* True of any disc, whatever region: the movies fill RAGE.STR end to end. */
static void CheckShape(const char *disc, const DiscStreamTable *table) {
    int index;
    if (table->offset[0] != 0)
        Fail(disc, "the first movie does not start RAGE.STR");
    for (index = 1; index < RAGE_DISC_STREAM_COUNT; index++) {
        if (table->offset[index] <= table->offset[index - 1])
            Fail(disc, "the movies are not in order");
    }
    for (index = 0; index < RAGE_DISC_STREAM_COUNT - 1; index++) {
        if (table->span[index] !=
            table->offset[index + 1] - table->offset[index])
            Fail(disc, "a movie's span does not reach the next one");
    }
    if (table->span[RAGE_DISC_STREAM_COUNT - 1] == 0)
        Fail(disc, "the last movie spans no sectors");
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
        if (table->frames[index] == 0) Fail(disc, "a movie shows no frames");
    }
}

static void CheckAgainst(const char *disc, const DiscStreamTable *table,
                             const unsigned int *offsets,
                             const unsigned int *frames,
                             const unsigned int *spans) {
    int index;
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
        CheckUnsigned(disc, "offset", index, table->offset[index],
                      offsets[index]);
        CheckUnsigned(disc, "frame count", index, table->frames[index],
                      frames[index]);
        if (spans != NULL)
            CheckUnsigned(disc, "span", index, table->span[index], spans[index]);
    }
}

static int SameAsPal(const DiscStreamTable *table) {
    int index;
    for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
        if (table->offset[index] != g_RetailPalStreamTable.offset[index])
            return 0;
    }
    return 1;
}

static void CheckDisc(const char *path) {
    ImageReader reader;
    DiscIdentity identity;

    if (EndsWith(path, ".chd")) {
        printf("skipping %s: this test reads sectors out of a .bin or .cue\n",
               path);
        return;
    }
    if (!OpenDisc(path, &reader)) {
        Fail(path, "could not be opened");
        return;
    }
    if (!DiscIdentify(ReadImageSector, &reader, &identity)) {
        Fail(path, "could not be identified");
        fclose(reader.file);
        return;
    }
    if (!identity.tableValid) {
        fprintf(stderr, "FAIL: %s (%s): no stream table: %s\n", identity.boot,
                identity.region, identity.reason);
        failures++;
        fclose(reader.file);
        return;
    }
    CheckShape(identity.boot, &identity.table);
    if (strcmp(identity.region, "PAL") == 0) {
        /* The one disc whose answer is known: retail's own table. */
        CheckAgainst(identity.boot, &identity.table,
                     g_RetailPalStreamTable.offset,
                     g_RetailPalStreamTable.frames,
                     g_RetailPalStreamTable.span);
    } else if (strcmp(identity.boot, "SLUS_004.03") == 0) {
        CheckAgainst(identity.boot, &identity.table, NTSC_U_OFFSETS,
                     NTSC_U_FRAMES, NULL);
        if (SameAsPal(&identity.table))
            Fail(identity.boot, "an American disc reported the PAL offsets");
    }
    printf("%s (%s): read the stream table off the disc\n", identity.boot,
           identity.region);
    fclose(reader.file);
}

/* The places the disc tests look, in the order HostInitDisc looks in them. */
static int FindDisc(const char *source, char *path, size_t pathSize) {
    static const char *const names[] = {"RAGE_PORT_DISC_IMAGE",
                                        "RAGE_PORT_DISC_CUE"};
    size_t index;
    FILE *file;
    for (index = 0; index < sizeof(names) / sizeof(names[0]); index++) {
        const char *value = getenv(names[index]);
        if (value == NULL || value[0] == '\0') continue;
        snprintf(path, pathSize, "%s", value);
        file = fopen(path, "rb");
        if (file == NULL) return 0;
        fclose(file);
        return 1;
    }
    snprintf(path, pathSize, "%s/%s", source, DEFAULT_CUE);
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    fclose(file);
    return 1;
}

int main(int argc, char **argv) {
    const char *source = argc > 1 ? argv[1] : ".";
    const char *other = getenv("RAGE_PORT_NTSC_CUE");
    char path[2048];

    if (!FindDisc(source, path, sizeof(path))) {
        printf("no disc image. This test derives the movie table from a disc, "
               "which is not in the repository.\nPut one at %s under the "
               "source tree - a symlink to the directory holding the cue sheet "
               "and its track files is enough - or set RAGE_PORT_DISC_CUE to a "
               "cue sheet.\n", DEFAULT_CUE);
        return SKIP_EXIT_CODE;
    }
    CheckDisc(path);
    if (other != NULL && other[0] != '\0') CheckDisc(other);
    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    return 0;
}
