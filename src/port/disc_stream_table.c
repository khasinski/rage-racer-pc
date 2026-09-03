#include "disc_stream_table.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
int _strnicmp(const char *lhs, const char *rhs, unsigned long long count);
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

enum {
    /* Mode 2: the subheader, then the sector's user data. */
    RAGE_DISC_SUBHEADER = 16,
    RAGE_DISC_SUBMODE_AUDIO = 0x04,
};

static const unsigned int RAGE_DISC_STR_MAGIC = 0x80010160u;

/* Retail main.exe @ 0x8007C6A8.  The second word of each entry is the final
 * STR frame, despite sharing GameCdLoadEntry with the sector-sized RAGE.BIN
 * index; the spans are the distances between the offsets, with RAGE.STR's own
 * length closing the last one. */
const DiscStreamTable g_RetailPalStreamTable = {
    {0x0000, 0x2F10, 0x353D, 0x3B6A, 0x4197, 0x47C4,
     0x4DF1, 0x541E, 0x5A4B, 0x6078, 0x66A5},
    {1800, 150, 150, 150, 150, 150, 150, 150, 150, 150, 1500},
    {0x2F10,
     0x062D, 0x062D, 0x062D, 0x062D, 0x062D,
     0x062D, 0x062D, 0x062D, 0x062D,
     0x3B40},
};

typedef DiscIsoReader DiscReader;
typedef DiscIsoFile DiscFile;

static unsigned int Le16(const unsigned char *data) {
    return (unsigned int)data[0] | ((unsigned int)data[1] << 8);
}

static unsigned int Le32(const unsigned char *data) {
    return Le16(data) | (Le16(data + 2) << 16);
}

/* "SLUS_004.03": four letters, an underscore, then three digits, a dot and two
 * more.  Nothing else in a PlayStation root directory looks like that. */
static int LooksLikeSerial(const unsigned char *name, unsigned int length) {
    unsigned int index;
    if (length < 11) return 0;
    for (index = 0; index < 4; index++) {
        if (name[index] < 'A' || name[index] > 'Z') return 0;
    }
    if (name[4] != '_' || name[8] != '.') return 0;
    for (index = 5; index < 11; index++) {
        if (index == 8) continue;
        if (name[index] < '0' || name[index] > '9') return 0;
    }
    return length == 11 || name[11] == ';';
}

typedef struct BootNameSearch {
    char *boot;
    int found;
} BootNameSearch;

static int FindBootNameVisitor(void *context, const unsigned char *name,
                               unsigned int length,
                               const DiscIsoFile *file) {
    BootNameSearch *search = context;
    (void)file;

    if (!LooksLikeSerial(name, length)) return 0;
    memcpy(search->boot, name, 11);
    search->boot[11] = '\0';
    search->found = 1;
    return 1;
}

static int FindBootLikeName(DiscReader *reader, char *boot, size_t bootSize) {
    BootNameSearch search;

    if (bootSize < 12) return 0;
    search.boot = boot;
    search.found = 0;
    return DiscIsoVisitRoot(reader, FindBootNameVisitor, &search) &&
           search.found;
}

/* SYSTEM.CNF's BOOT line names the executable, which is the disc's serial:
 * "BOOT = cdrom:\SLUS_004.03;1". */
static int ParseBootName(const unsigned char *cnf, unsigned int size,
                         char *boot, size_t bootSize) {
    unsigned int index;
    for (index = 0; index + 4 <= size; index++) {
        unsigned int cursor;
        size_t written = 0;
        if (strncasecmp((const char *)cnf + index, "BOOT", 4) != 0) continue;
        if (index != 0 && cnf[index - 1] != '\n' && cnf[index - 1] != '\r')
            continue;
        cursor = index + 4;
        while (cursor < size && (cnf[cursor] == ' ' || cnf[cursor] == '\t'))
            cursor++;
        if (cursor >= size || cnf[cursor] != '=') continue;
        cursor++;
        while (cursor < size && (cnf[cursor] == ' ' || cnf[cursor] == '\t'))
            cursor++;
        /* Step over "cdrom:" and any path separators before the name. */
        if (cursor + 6 <= size &&
            strncasecmp((const char *)cnf + cursor, "cdrom:", 6) == 0)
            cursor += 6;
        while (cursor < size && (cnf[cursor] == '\\' || cnf[cursor] == '/'))
            cursor++;
        while (cursor < size && cnf[cursor] > ' ' && cnf[cursor] != ';') {
            if (written + 1 >= bootSize) return 0;
            boot[written++] = (char)cnf[cursor++];
        }
        boot[written] = '\0';
        return written > 0;
    }
    return 0;
}

const char *DiscRegionForBootName(const char *boot) {
    if (boot == NULL) return "unknown";
    if (strncasecmp(boot, "SCES", 4) == 0 || strncasecmp(boot, "SLES", 4) == 0 ||
        strncasecmp(boot, "SCED", 4) == 0)
        return "PAL";
    if (strncasecmp(boot, "SCUS", 4) == 0 || strncasecmp(boot, "SLUS", 4) == 0)
        return "NTSC-U";
    if (strncasecmp(boot, "SCPS", 4) == 0 || strncasecmp(boot, "SLPS", 4) == 0 ||
        strncasecmp(boot, "SLPM", 4) == 0 || strncasecmp(boot, "SCPM", 4) == 0)
        return "NTSC-J";
    return "unknown";
}

/* Walks RAGE.STR's chunk headers looking for the eleven movies.  The file has
 * no table of contents; a movie ends where the frame numbering restarts. */
static int ScanStreamStarts(DiscReader *reader, const DiscFile *stream,
                            unsigned int *starts, unsigned int *total) {
    unsigned int sectors = stream->size / DISC_ISO_SECTOR_SIZE +
                           (stream->size % DISC_ISO_SECTOR_SIZE != 0);
    unsigned int index;
    unsigned int found = 0;
    unsigned int previous = 0;
    int seenFrame = 0;
    unsigned char raw[DISC_RAW_SECTOR_SIZE];

    *total = sectors;
    for (index = 0; index < sectors; index++) {
        const unsigned char *body;
        unsigned int chunk;
        unsigned int frame;
        if (stream->lba > UINT_MAX - index ||
            !reader->read(reader->context, stream->lba + index, raw)) {
            return 0;
        }
        if (raw[RAGE_DISC_SUBHEADER + 2] & RAGE_DISC_SUBMODE_AUDIO) continue;
        body = raw + DISC_MODE2_USER_OFFSET;
        if (Le32(body) != RAGE_DISC_STR_MAGIC) continue;
        chunk = Le16(body + 4);
        frame = Le32(body + 8);
        if (!seenFrame || (chunk == 0 && frame < previous)) {
            if (found == RAGE_DISC_STREAM_COUNT) return 0;
            starts[found++] = index;
        }
        previous = frame;
        seenFrame = 1;
    }
    return found == RAGE_DISC_STREAM_COUNT;
}

/* The game's own stream table is eleven eight-byte entries, a sector offset
 * beside the last frame the game shows.  Finding the offsets in the executable
 * is what identifies it, and the frame counts sit next to them. */
static int FindStreamTable(const unsigned char *executable, unsigned int size,
                           const unsigned int *starts,
                           unsigned int *frames) {
    unsigned int offset;
    unsigned int entries = RAGE_DISC_STREAM_COUNT * 8;
    if (size < entries) return 0;
    for (offset = 0; offset <= size - entries; offset += 4) {
        unsigned int index;
        for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
            if (Le32(executable + offset + index * 8) != starts[index]) break;
        }
        if (index < RAGE_DISC_STREAM_COUNT) continue;
        for (index = 0; index < RAGE_DISC_STREAM_COUNT; index++) {
            unsigned int count = Le32(executable + offset + index * 8 + 4);
            if (count == 0 || count > 100000) break;
            frames[index] = count;
        }
        if (index == RAGE_DISC_STREAM_COUNT) return 1;
    }
    return 0;
}

static int StartsAreSane(const unsigned int *starts, unsigned int total) {
    unsigned int index;
    if (starts[0] != 0) return 0;
    for (index = 1; index < RAGE_DISC_STREAM_COUNT; index++) {
        if (starts[index] <= starts[index - 1]) return 0;
    }
    return starts[RAGE_DISC_STREAM_COUNT - 1] < total;
}

static int ReadStreamTable(DiscReader *reader, DiscIdentity *identity,
                           const char *boot) {
    DiscFile stream;
    DiscFile executable;
    unsigned char *blob;
    unsigned int total = 0;
    unsigned int index;
    int found;

    if (reader->userOffset != DISC_MODE2_USER_OFFSET) {
        identity->reason = "the disc has no Mode 2 track to read movies from";
        return 0;
    }
    if (!DiscIsoFindFile(reader, "RAGE.STR", &stream)) {
        identity->reason = "the disc has no RAGE.STR";
        return 0;
    }
    if (!ScanStreamStarts(reader, &stream, identity->table.offset, &total) ||
        !StartsAreSane(identity->table.offset, total)) {
        identity->reason = "RAGE.STR does not hold eleven movies";
        return 0;
    }
    if (!DiscIsoFindFile(reader, boot, &executable)) {
        identity->reason = "the boot executable named in SYSTEM.CNF is missing";
        return 0;
    }
    blob = DiscIsoReadWholeFile(reader, &executable);
    if (blob == NULL) {
        identity->reason = "the boot executable could not be read";
        return 0;
    }
    found = FindStreamTable(blob, executable.size, identity->table.offset,
                            identity->table.frames);
    free(blob);
    if (!found) {
        identity->reason = "the boot executable holds no matching stream table";
        return 0;
    }
    for (index = 0; index < RAGE_DISC_STREAM_COUNT - 1; index++) {
        identity->table.span[index] =
            identity->table.offset[index + 1] - identity->table.offset[index];
    }
    identity->table.span[RAGE_DISC_STREAM_COUNT - 1] =
        total - identity->table.offset[RAGE_DISC_STREAM_COUNT - 1];
    identity->tableValid = 1;
    return 1;
}

int DiscIdentify(DiscRawSectorReader read, void *context,
                 DiscIdentity *identity) {
    DiscReader reader;
    DiscFile config;
    unsigned char *cnf;
    int named;

    if (identity == NULL) return 0;
    memset(identity, 0, sizeof(*identity));
    identity->region = "unknown";
    identity->reason = "the disc could not be identified";
    if (!DiscIsoOpen(&reader, read, context)) return 0;

    named = 0;
    if (DiscIsoFindFile(&reader, "SYSTEM.CNF", &config)) {
        cnf = DiscIsoReadWholeFile(&reader, &config);
        if (cnf != NULL) {
            named = ParseBootName(cnf, config.size, identity->boot,
                                  sizeof(identity->boot));
            free(cnf);
        }
    }
    /* A disc with no readable SYSTEM.CNF still names its executable in the
     * root directory, and a PlayStation serial has a shape of its own. */
    if (!named) named = FindBootLikeName(&reader, identity->boot,
                                         sizeof(identity->boot));
    if (!named) {
        identity->reason = "the disc names no boot executable";
        return 1;
    }
    identity->region = DiscRegionForBootName(identity->boot);
    if (ReadStreamTable(&reader, identity, identity->boot)) {
        identity->reason = NULL;
    }
    return 1;
}
