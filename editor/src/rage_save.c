#include "rage_save.h"

#include <ctype.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(GameSaveBlock) == MC_BLOCK_SIZE,
               "the editor must use the game's payload layout");
_Static_assert(sizeof(GameSaveHeaderRow) == 0x80,
               "the editor must use the game's header layout");
_Static_assert(sizeof(RageSaveFile) == RAGE_SAVE_FILE_SIZE,
               "the file is an icon block, a header, the payload and the "
               "header again, with nothing between them");

/*
 * The PAL name is the one the port carries in g_SaveFilePath, and the
 * American serial is the one its disc table recognises. The Japanese release
 * is here because the editor is asked to read all three, but nothing in this
 * repository states its serial, so it is marked unverified and the file name
 * can be overridden.
 */
static const RageRegionInfo kRegions[] = {
    {RAGE_REGION_PAL, "PAL", "SCES-006.50", "BESCES-00650", 1},
    {RAGE_REGION_NTSC_U, "NTSC-U", "SLUS-004.03", "BASLUS-00403", 1},
    {RAGE_REGION_NTSC_J, "NTSC-J", "SLPS-00744", "BISLPS-00744", 0},
};

/*
 * Both name tables come from the port's own content options, which carry them
 * because the Japanese release renames five cars; the makers come from the
 * series' published car lists.
 */
static const char *const kCarNames[2][13] = {
    {"Erriso", "Abeille", "Pegase", "Esperanza", "Acceron", "Bayonet",
     "Hijack", "Fatalita", "Istante", "Ghepardo", "Vainqure", "Bulshade",
     "Squaldon"},
    {"Alouette", "Abeille", "Pegase", "Esperanza", "Instinct", "Bayonet",
     "Hijack", "Fatalita", "Istante", "Ghepardo", "Victoire", "Tempest",
     "Dragone"},
};

static const char *const kCarMakers[13] = {
    "Age", "Age", "Age", "Gnade", "Lizard", "Lizard", "Lizard",
    "Assoluto", "Assoluto", "Assoluto", "Age", "Lizard", "Assoluto",
};

const char *RageCarName(int index, RageRegion region) {
    if (index < 0 || index >= 13) return NULL;
    return kCarNames[region == RAGE_REGION_NTSC_J ? 1 : 0][index];
}

const char *RageCarMaker(int index) {
    if (index < 0 || index >= 13) return NULL;
    return kCarMakers[index];
}

const char kRageNameCharset[] = "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ.-!?@";

const RageRegionInfo *RageRegionTable(size_t *count) {
    if (count != NULL) *count = sizeof(kRegions) / sizeof(kRegions[0]);
    return kRegions;
}

const RageRegionInfo *RageRegionFind(RageRegion region) {
    size_t i;
    for (i = 0; i < sizeof(kRegions) / sizeof(kRegions[0]); i++) {
        if (kRegions[i].region == region) return &kRegions[i];
    }
    return NULL;
}

static const char *BaseName(const char *path) {
    const char *name = path;
    const char *cursor;

    for (cursor = path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') name = cursor + 1;
    }
    return name;
}

RageRegion RageRegionFromPath(const char *path) {
    const char *name;
    size_t i;

    if (path == NULL) return RAGE_REGION_UNKNOWN;
    name = BaseName(path);
    for (i = 0; i < sizeof(kRegions) / sizeof(kRegions[0]); i++) {
        size_t length = strlen(kRegions[i].cardPrefix);
        if (strncmp(name, kRegions[i].cardPrefix, length) == 0)
            return kRegions[i].region;
    }
    /*
     * An unrecognised serial still says which territory it came from: the
     * second letter of a card file name is the region, E for Europe, A for
     * America and I for Japan.
     */
    if (name[0] == 'B') {
        if (name[1] == 'E') return RAGE_REGION_PAL;
        if (name[1] == 'A') return RAGE_REGION_NTSC_U;
        if (name[1] == 'I') return RAGE_REGION_NTSC_J;
    }
    return RAGE_REGION_UNKNOWN;
}

int RageRegionCardName(RageRegion region, int slot, char *out, size_t size) {
    const RageRegionInfo *info = RageRegionFind(region);

    if (info == NULL || out == NULL || slot < 0 || slot >= RAGE_SAVE_SLOTS)
        return 0;
    return snprintf(out, size, "%s RAGE%03d", info->cardPrefix, slot) > 0;
}

/* Both sums add the file up as little-endian halfwords and complement it. */
static unsigned int ComplementHalfwordSum(const void *data, size_t bytes) {
    const unsigned char *cursor = data;
    unsigned int sum = 0;
    size_t offset;

    for (offset = 0; offset + 1 < bytes; offset += 2) {
        sum += (unsigned int)cursor[offset] |
               ((unsigned int)cursor[offset + 1] << 8);
    }
    return ~sum;
}

unsigned int RageSaveHeaderChecksum(const GameSaveHeaderRow *row) {
    return ComplementHalfwordSum(row->bytes, 0x7C);
}

unsigned int RageSaveBlockChecksum(const GameSaveBlock *block) {
    return ComplementHalfwordSum(block, MC_BLOCK_CHECKSUM_OFS);
}

void RageSaveRefresh(RageSaveFile *save) {
    if (save == NULL) return;
    save->block.checksum = RageSaveBlockChecksum(&save->block);
    save->header.fields.checksum = RageSaveHeaderChecksum(&save->header);
    /* The second copy is the first one again, checksum included. */
    save->trailer = save->header;
}

void RageSaveCheck(const RageSaveFile *save, RageSaveReport *report) {
    if (save == NULL || report == NULL) return;
    report->iconRecognised = save->icon[0] == 'S' && save->icon[1] == 'C';
    report->headerChecksumValid =
        save->header.fields.checksum == RageSaveHeaderChecksum(&save->header);
    report->blockChecksumValid =
        save->block.checksum == RageSaveBlockChecksum(&save->block);
    report->trailerChecksumValid =
        save->trailer.fields.checksum == RageSaveHeaderChecksum(&save->trailer);
    report->trailerMatchesHeader =
        memcmp(&save->header, &save->trailer, sizeof(save->header)) == 0;
}

int RageSaveLoad(const char *path, RageSaveFile *save, RageSaveReport *report) {
    FILE *stream;
    long size;
    size_t read;

    if (path == NULL || save == NULL || report == NULL) return 0;
    memset(report, 0, sizeof(*report));
    report->region = RageRegionFromPath(path);

    stream = fopen(path, "rb");
    if (stream == NULL) {
        report->status = RAGE_SAVE_UNREADABLE;
        snprintf(report->detail, sizeof(report->detail),
                 "cannot open %s", BaseName(path));
        return 0;
    }
    if (fseek(stream, 0, SEEK_END) != 0 || (size = ftell(stream)) < 0 ||
        fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        report->status = RAGE_SAVE_UNREADABLE;
        snprintf(report->detail, sizeof(report->detail),
                 "cannot measure %s", BaseName(path));
        return 0;
    }
    if (size != RAGE_SAVE_FILE_SIZE) {
        fclose(stream);
        report->status = RAGE_SAVE_WRONG_SIZE;
        if (size == 8192 || size == 8320) {
            snprintf(report->detail, sizeof(report->detail),
                     "%ld bytes: this looks like a whole memory card block, "
                     "not one save file", size);
        } else {
            snprintf(report->detail, sizeof(report->detail),
                     "%ld bytes, expected %d", size, RAGE_SAVE_FILE_SIZE);
        }
        return 0;
    }
    read = fread(save, 1, sizeof(*save), stream);
    fclose(stream);
    if (read != sizeof(*save)) {
        report->status = RAGE_SAVE_UNREADABLE;
        snprintf(report->detail, sizeof(report->detail),
                 "read %zu of %d bytes", read, RAGE_SAVE_FILE_SIZE);
        return 0;
    }

    RageSaveCheck(save, report);
    if (!report->iconRecognised) {
        /*
         * Everything else about the file can be wrong and still be worth
         * editing, but without the card signature there is no reason to
         * believe the bytes mean what this program thinks they mean.
         */
        report->status = RAGE_SAVE_NOT_A_SAVE;
        snprintf(report->detail, sizeof(report->detail),
                 "no memory card signature: first two bytes are %02X %02X, "
                 "expected 'S' 'C'", save->icon[0], save->icon[1]);
        return 0;
    }
    report->status = RAGE_SAVE_OK;
    return 1;
}

int RageSaveStore(const char *path, RageSaveFile *save,
                  RageSaveReport *report) {
    FILE *stream;

    if (path == NULL || save == NULL) return 0;
    RageSaveRefresh(save);
    stream = fopen(path, "wb");
    if (stream == NULL) {
        if (report != NULL) {
            report->status = RAGE_SAVE_UNREADABLE;
            snprintf(report->detail, sizeof(report->detail),
                     "cannot write %s", BaseName(path));
        }
        return 0;
    }
    if (fwrite(save, 1, sizeof(*save), stream) != sizeof(*save) ||
        fclose(stream) != 0) {
        if (report != NULL) {
            report->status = RAGE_SAVE_UNREADABLE;
            snprintf(report->detail, sizeof(report->detail),
                     "failed to write %s", BaseName(path));
        }
        return 0;
    }
    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->status = RAGE_SAVE_OK;
        report->region = RageRegionFromPath(path);
        RageSaveCheck(save, report);
    }
    return 1;
}

size_t RageNameCharsetSize(void) { return strlen(kRageNameCharset); }

void RageSaveReadTeamName(const GameSaveHeaderRow *row, char *out,
                          size_t size) {
    size_t charset = RageNameCharsetSize();
    size_t length;
    size_t i;

    if (out == NULL || size == 0) return;
    out[0] = '\0';
    if (row == NULL) return;
    length = row->fields.nameLength;
    if (length > RAGE_TEAM_NAME_LENGTH) length = RAGE_TEAM_NAME_LENGTH;
    if (length + 1 > size) length = size - 1;
    for (i = 0; i < length; i++) {
        unsigned char index = row->fields.name[i];
        out[i] = index < charset ? kRageNameCharset[index] : '?';
    }
    out[length] = '\0';
}

void RageSaveWriteTeamName(GameSaveHeaderRow *row, const char *text) {
    size_t charset = RageNameCharsetSize();
    size_t written = 0;
    size_t i;

    if (row == NULL || text == NULL) return;
    for (i = 0; text[i] != '\0' && written < RAGE_TEAM_NAME_LENGTH; i++) {
        int upper = toupper((unsigned char)text[i]);
        const char *found = memchr(kRageNameCharset, upper, charset);
        /* Anything the game cannot spell becomes a space rather than a hole. */
        row->fields.name[written++] =
            (unsigned char)(found != NULL ? found - kRageNameCharset : 10);
    }
    for (i = written; i < RAGE_TEAM_NAME_LENGTH; i++) row->fields.name[i] = 0;
    row->fields.nameLength = (unsigned char)written;
}

/* Four pixels share a halfword, low nibble leftmost. */
static int LogoNibble(int x, int y, int *shift) {
    int column = x >> 2;
    *shift = (x - column * 4) * 4;
    return y * 16 + column;
}

int RageLogoPixel(const GameSaveBlock *block, int x, int y) {
    int shift;
    int index;

    if (block == NULL || x < 0 || y < 0 || x >= RAGE_LOGO_WIDTH ||
        y >= RAGE_LOGO_HEIGHT)
        return 0;
    index = LogoNibble(x, y, &shift);
    return (block->teamLogoCanvas[index] >> shift) & 0xF;
}

void RageLogoSetPixel(GameSaveBlock *block, int x, int y, int colour) {
    int shift;
    int index;
    unsigned short mask;

    if (block == NULL || x < 0 || y < 0 || x >= RAGE_LOGO_WIDTH ||
        y >= RAGE_LOGO_HEIGHT)
        return;
    index = LogoNibble(x, y, &shift);
    mask = (unsigned short)(0xF << shift);
    block->teamLogoCanvas[index] = (unsigned short)(
        (block->teamLogoCanvas[index] & ~mask) | ((colour & 0xF) << shift));
}

/* Five bits a channel, blue highest, and a top bit the hardware reads as
 * transparency rather than as colour. */
void RageLogoColour(unsigned short entry, unsigned char *rgb, int *transparent) {
    if (rgb != NULL) {
        unsigned int r = entry & 0x1F;
        unsigned int g = (entry >> 5) & 0x1F;
        unsigned int b = (entry >> 10) & 0x1F;
        rgb[0] = (unsigned char)((r << 3) | (r >> 2));
        rgb[1] = (unsigned char)((g << 3) | (g >> 2));
        rgb[2] = (unsigned char)((b << 3) | (b >> 2));
    }
    if (transparent != NULL) *transparent = (entry & 0x8000) != 0;
}

unsigned short RageLogoPackColour(const unsigned char *rgb, int transparent) {
    unsigned int r, g, b;

    if (rgb == NULL) return 0;
    r = rgb[0] >> 3;
    g = rgb[1] >> 3;
    b = rgb[2] >> 3;
    return (unsigned short)(r | (g << 5) | (b << 10) |
                            (transparent ? 0x8000 : 0));
}

/*
 * What the game itself holds after a fresh boot, rather than a block of
 * zeroes. Zeroes are a legal save but not an honest one: the volumes would be
 * silent, no car would be owned, every class record would read as a finished
 * first place, and the neGcon would have no play in its steering.
 */
static const unsigned char kCarDefaults[13][6] = {
    {0, 2, 0, 0, 0, 0}, {0, 3, 0, 1, 1, 0}, {0, 4, 1, 2, 2, 0},
    {0, 1, 0, 3, 3, 1}, {0, 0, 0, 4, 4, 0}, {0, 0, 0, 5, 5, 0},
    {0, 0, 1, 6, 6, 0}, {0, 2, 0, 7, 7, 0}, {0, 2, 1, 8, 8, 0},
    {0, 3, 1, 9, 9, 0}, {0, 4, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 0},
    {0, 3, 1, 0, 0, 0},
};

void RageSaveInit(RageSaveFile *save, RageRegion region, int slot) {
    int garage;
    int car;
    int i;

    if (save == NULL) return;
    memset(save, 0, sizeof(*save));
    (void)region;
    (void)slot;

    /* What the console looks for when it lists a card. */
    save->icon[0] = 'S';
    save->icon[1] = 'C';
    save->icon[2] = 0x11;
    save->icon[3] = 1;
    RageSaveWriteTeamName(&save->header, "RAGE");

    /* The one calibration value the game does not start at zero: without it
     * the steering has no dead zone at all. */
    save->block.negconSteerPlay = 1;

    /* The Gnade is the car you begin with, and the only one owned. */
    for (garage = 0; garage < 3; garage++) {
        for (car = 0; car < 13; car++) {
            SavedCarSetup *setup = &save->block.carSetup[garage][car];
            setup->modelVariant = kCarDefaults[car][0];
            setup->tireCompound = kCarDefaults[car][1];
            setup->transmission = kCarDefaults[car][2];
            setup->paintColor1 = kCarDefaults[car][3];
            setup->paintColor2 = kCarDefaults[car][4];
            setup->enabled = kCarDefaults[car][5];
        }
    }
    save->block.grandPrixProgress.carIndex = 3;
    save->block.extraGrandPrixProgress.carIndex = 3;
    save->block.timeAttackProgress.carIndex = 3;
    /* The grand prix has not been entered; time attack counts from zero. */
    save->block.grandPrixProgress.maxClassReached = -1;
    save->block.extraGrandPrixProgress.maxClassReached = -1;

    /* An empty class record is -1, not a first place. */
    for (i = 1; i < 11; i++) save->block.classRecords[i].grade = 0xFFFF;

    /* Five retries, and no course finished. */
    save->block.grandPrixCourseProgress[3] = 0xFF;
    save->block.extraGrandPrixCourseProgress[3] = 0xFF;
    save->block.grandPrixCourseProgress[6] = 5;
    save->block.extraGrandPrixCourseProgress[6] = 5;

    save->block.bgmVolume = 0xF;
    save->block.sfxVolume = 0xF;

    RageSaveRefresh(save);
}


int RageSaveSlotFromPath(const char *path) {
    const char *name;
    size_t length;

    if (path == NULL) return -1;
    name = BaseName(path);
    length = strlen(name);
    /* The game names its files "... RAGE000" through "... RAGE002": four
     * letters and three digits, seven characters at the end. */
    if (length < 7) return -1;
    if (strncmp(name + length - 7, "RAGE", 4) != 0) return -1;
    if (!isdigit((unsigned char)name[length - 1])) return -1;
    return name[length - 1] - '0';
}

int RageSaveCardDirectory(int card, char *out, size_t size) {
    const char *base;
    int written;

    if (out == NULL || card < 0 || card > 1) return 0;
#ifdef _WIN32
    base = getenv("APPDATA");
    if (base == NULL || base[0] == '\0') return 0;
    written = snprintf(out, size, "%s\\Rage Racer\\bu%d0", base, card);
#elif defined(__APPLE__)
    base = getenv("HOME");
    if (base == NULL || base[0] == '\0') return 0;
    written = snprintf(out, size,
                       "%s/Library/Application Support/Rage Racer/bu%d0", base,
                       card);
#else
    base = getenv("XDG_STATE_HOME");
    if (base != NULL && base[0] != '\0') {
        written = snprintf(out, size, "%s/rage-racer/bu%d0", base, card);
    } else {
        base = getenv("HOME");
        if (base == NULL || base[0] == '\0') return 0;
        written = snprintf(out, size, "%s/.local/state/rage-racer/bu%d0", base,
                           card);
    }
#endif
    return written > 0 && (size_t)written < size;
}

static void DescribeEntry(const char *path, RageSaveEntry *entry) {
    RageSaveFile save;
    RageSaveReport report;

    snprintf(entry->path, sizeof(entry->path), "%s", path);
    entry->region = RageRegionFromPath(path);
    entry->slot = RageSaveSlotFromPath(path);
    entry->team[0] = '\0';
    entry->money = 0;
    entry->valid = 0;
    if (!RageSaveLoad(path, &save, &report)) return;
    RageSaveReadTeamName(&save.header, entry->team, sizeof(entry->team));
    entry->money = save.block.grandPrixProgress.money;
    entry->valid = report.headerChecksumValid && report.blockChecksumValid;
    if (report.region != RAGE_REGION_UNKNOWN) entry->region = report.region;
}

static int ScanDirectory(const char *directory, RageSaveEntry *entries,
                         int found, int max) {
    char path[1024];
#ifdef _WIN32
    WIN32_FIND_DATAA data;
    HANDLE search;

    snprintf(path, sizeof(path), "%s\\*", directory);
    search = FindFirstFileA(path, &data);
    if (search == INVALID_HANDLE_VALUE) return found;
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        if (found >= max) break;
        snprintf(path, sizeof(path), "%s\\%s", directory, data.cFileName);
        DescribeEntry(path, &entries[found++]);
    } while (FindNextFileA(search, &data));
    FindClose(search);
#else
    DIR *handle = opendir(directory);
    struct dirent *item;

    if (handle == NULL) return found;
    while ((item = readdir(handle)) != NULL && found < max) {
        if (item->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", directory, item->d_name);
        DescribeEntry(path, &entries[found++]);
    }
    closedir(handle);
#endif
    return found;
}

int RageSaveDiscover(RageSaveEntry *entries, int max) {
    char directory[1024];
    int found = 0;
    int card;

    if (entries == NULL || max <= 0) return 0;
    for (card = 0; card < 2 && found < max; card++) {
        if (RageSaveCardDirectory(card, directory, sizeof(directory)))
            found = ScanDirectory(directory, entries, found, max);
    }
    return found;
}
