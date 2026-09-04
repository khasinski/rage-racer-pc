#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
/* WinAPI turns OpenEvent into a macro that collides with the PSY-Q function. */
#ifdef OpenEvent
#undef OpenEvent
#endif
#define access _access
#ifndef R_OK
#define R_OK 4
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#else
#include <unistd.h>
#endif

#include <psyz/cd.h>

#include "host_disc.h"
#include "archive_index.h"
#include "disc_cue.h"
#include "disc_discovery.h"
#include "disc_iso.h"
#include "disc_raw_file.h"
#include "disc_picker.h"
#include "platform_paths.h"
#include "runtime_config.h"
#include "chd_disc.h"
#include "disc_stream_table.h"
#include "fmv_audio.h"

typedef struct RageHostDisc {
    FILE *file;
    int chd;
    long track_offset;
    DiscIsoFile archive;
    DiscIsoFile stream;
    int user_offset;
    char source_path[PATH_MAX];
} RageHostDisc;

static RageHostDisc g_RageHostDisc;
static const char *s_DiscRegion = "unknown";

static void HostResetCdBackend(void) {
    ChdClose();
    Psyz_CdSetDiskPath(NULL);
    g_RageHostDisc.chd = 0;
}

static void HostCloseDisc(void) {
    if (g_RageHostDisc.file != NULL) fclose(g_RageHostDisc.file);
    HostResetCdBackend();
    memset(&g_RageHostDisc, 0, sizeof(g_RageHostDisc));
    s_DiscRegion = "unknown";
}

const char *HostDiscRegion(void) { return s_DiscRegion; }

static int HostMakeDiscConfigPath(char *path, size_t size) {
    int written;

    /* Keep the historical key so existing installations retain their choice. */
    if (PlatformUserConfigPath("disc-cue-path", path, size)) return 1;
    if (path == NULL || size == 0) return 0;
    written = snprintf(path, size, "%s", "disc-cue-path");
    if (written < 0 || (size_t)written >= size) {
        path[0] = '\0';
        return 0;
    }
    return 1;
}

static int HostLoadSavedDiscPath(char *path, size_t size) {
    char configPath[PATH_MAX];

    return HostMakeDiscConfigPath(configPath, sizeof(configPath)) &&
           DiscReadSavedPath(configPath, path, size);
}

static void HostSaveDiscPath(const char *discPath) {
    char directory[PATH_MAX];
    char path[PATH_MAX];

    if (!PlatformUserConfigDirectory(directory, sizeof(directory)) ||
        !PlatformEnsureDirectory(directory)) return;
    if (HostMakeDiscConfigPath(path, sizeof(path)))
        DiscWriteSavedPath(path, discPath);
}

/* The picker asks the desktop for a path; whether it names a disc this build
 * can read is decided here. */
static int HostChooseDisc(char *path, size_t size) {
    return HostShowDiscPicker(path, size) && DiscPathIsSupportedImage(path) &&
           access(path, R_OK) == 0;
}

static int HostReadRawSector(void *context, unsigned int sector,
                             unsigned char *raw) {
    DiscRawFile disc;

    (void)context;
    if (raw == NULL) return 0;
    if (g_RageHostDisc.chd) {
        return ChdReadRawSector(sector, raw);
    }
    disc.file = g_RageHostDisc.file;
    disc.trackOffset = g_RageHostDisc.track_offset;
    return DiscRawFileReadSector(&disc, sector, raw);
}

static int HostReadUserSector(unsigned int sector, unsigned char *user) {
    DiscIsoReader reader = {
        HostReadRawSector,
        NULL,
        g_RageHostDisc.user_offset,
    };

    return DiscIsoReadUserSector(&reader, sector, user);
}

static int HostFindArchive(void) {
    DiscIsoReader reader;
    DiscIsoFile archive;
    DiscIsoFile stream;

    if (!DiscIsoOpen(&reader, HostReadRawSector, NULL) ||
        !DiscIsoFindFile(&reader, "RAGE.BIN", &archive) ||
        !DiscIsoFindFile(&reader, "RAGE.STR", &stream)) {
        return 0;
    }
    g_RageHostDisc.user_offset = reader.userOffset;
    g_RageHostDisc.archive = archive;
    g_RageHostDisc.stream = stream;
    return 1;
}

/* How many sectors of RAGE.STR each movie occupies.  The retail PAL values
 * stand until a mounted disc says otherwise, and are what a disc that cannot
 * be read falls back to. */
static unsigned int s_StreamSpans[RAGE_DISC_STREAM_COUNT];

unsigned int HostStreamSectorSpan(int stream) {
    if (stream < 0 || stream >= RAGE_DISC_STREAM_COUNT) return 0;
    return s_StreamSpans[stream];
}

/* g_StreamCdEntries holds retail's PAL table until a disc is mounted, so a run
 * that never gets one behaves as the PAL build always did. */
static void HostSeedStreamTable(void) {
    int stream;
    DiscStreamTablePublish(&g_RetailPalStreamTable);
    for (stream = 0; stream < RAGE_DISC_STREAM_COUNT; stream++) {
        s_StreamSpans[stream] = g_RetailPalStreamTable.span[stream];
    }
}

/* Where the movies are and how long they run is a property of the disc in the
 * drive, not of the build.  Read it off the disc that was mounted, and say so
 * once, because a wrong table shows up as movies that start mid-scene and stop
 * halfway through rather than as an error. */
static void HostAdoptDiscStreamTable(void) {
    DiscIdentity identity;
    int stream;

    if (!DiscIdentify(HostReadRawSector, NULL, &identity)) {
        fprintf(stderr, "rage-port: disc region unknown, keeping the "
                        "built-in PAL FMV table\n");
        return;
    }
    s_DiscRegion = identity.region;
    fprintf(stderr, "rage-port: disc %s region=%s\n",
            identity.boot[0] != '\0' ? identity.boot : "unidentified",
            identity.region);
    if (!identity.tableValid) {
        fprintf(stderr,
                "rage-port: keeping the built-in PAL FMV table: %s\n",
                identity.reason != NULL ? identity.reason : "unknown reason");
        return;
    }
    DiscStreamTablePublish(&identity.table);
    for (stream = 0; stream < RAGE_DISC_STREAM_COUNT; stream++) {
        s_StreamSpans[stream] = identity.table.span[stream];
    }
    fprintf(stderr, "rage-port: FMV table from %s:", identity.boot);
    for (stream = 0; stream < RAGE_DISC_STREAM_COUNT; stream++) {
        fprintf(stderr, " %04X/%u/%u", identity.table.offset[stream],
                identity.table.frames[stream], identity.table.span[stream]);
    }
    fprintf(stderr, "\n");
}

int HostReadStreamSector(unsigned int sector, unsigned char *raw) {
    unsigned int absoluteSector;

    if (raw == NULL || (!g_RageHostDisc.chd && g_RageHostDisc.file == NULL) ||
        !DiscIsoResolveSector(&g_RageHostDisc.stream, sector,
                              &absoluteSector)) {
        return 0;
    }
    return HostReadRawSector(NULL, absoluteSector, raw);
}

int HostStreamAbsoluteRange(unsigned int firstSector,
                            unsigned int sectorCount, int *absoluteFirst,
                            int *absoluteEnd) {
    unsigned int lastSector;
    unsigned int first;
    unsigned int last;

    if (sectorCount == 0 || absoluteFirst == NULL || absoluteEnd == NULL ||
        firstSector > UINT_MAX - (sectorCount - 1)) {
        return 0;
    }
    lastSector = firstSector + sectorCount - 1;
    if (!DiscIsoResolveSector(&g_RageHostDisc.stream, firstSector, &first) ||
        !DiscIsoResolveSector(&g_RageHostDisc.stream, lastSector, &last) ||
        first > INT_MAX || last >= INT_MAX) {
        return 0;
    }
    *absoluteFirst = (int)first;
    *absoluteEnd = (int)last + 1;
    return 1;
}

static int HostReadArchive(unsigned int offset, void *destination,
                           unsigned int size) {
    unsigned char sector[DISC_ISO_SECTOR_SIZE];
    unsigned char *output = destination;
    FILE *test_archive;

    if (destination == NULL) return 0;
    if (g_RageHostDisc.file == NULL && !g_RageHostDisc.chd &&
        RuntimeConfigEnabled("runtime.test_mode")) {
        size_t loaded;
        test_archive = fopen("assets/PAL/RAGE.BIN", "rb");
        if (test_archive == NULL || fseek(test_archive, (long)offset, SEEK_SET) != 0) {
            if (test_archive != NULL) fclose(test_archive);
            return 0;
        }
        loaded = fread(destination, 1, size, test_archive);
        fclose(test_archive);
        return loaded == size;
    }
    if (offset > g_RageHostDisc.archive.size ||
        size > g_RageHostDisc.archive.size - offset) {
        return 0;
    }
    while (size > 0) {
        unsigned int sector_offset = offset % DISC_ISO_SECTOR_SIZE;
        unsigned int chunk = DISC_ISO_SECTOR_SIZE - sector_offset;
        if (chunk > size) chunk = size;
        unsigned int absoluteSector;

        if (!DiscIsoResolveSector(
                &g_RageHostDisc.archive,
                offset / DISC_ISO_SECTOR_SIZE, &absoluteSector) ||
            !HostReadUserSector(absoluteSector, sector)) {
            return 0;
        }
        memcpy(output, sector + sector_offset, chunk);
        output += chunk;
        offset += chunk;
        size -= chunk;
    }
    return 1;
}

static int HostOpenDataTrack(const char *path) {
    int written;

    g_RageHostDisc.file = fopen(path, "rb");
    if (g_RageHostDisc.file != NULL && HostFindArchive()) {
        written = snprintf(g_RageHostDisc.source_path,
                           sizeof(g_RageHostDisc.source_path), "%s", path);
        if (written >= 0 &&
            (size_t)written < sizeof(g_RageHostDisc.source_path)) {
            return 1;
        }
    }
    if (g_RageHostDisc.file != NULL) fclose(g_RageHostDisc.file);
    g_RageHostDisc.file = NULL;
    return 0;
}

/* Resolves a selected image all the way to a readable archive, so a path that
 * no longer works is reported before the game starts loading assets. */
static int HostOpenDiscImage(const char *discPath) {
    char dataTrackPath[PATH_MAX];

    HostResetCdBackend();
    if (DiscPathIsChd(discPath)) {
        int written;

        if (!ChdOpen(discPath)) return 0;
        g_RageHostDisc.chd = 1;
        g_RageHostDisc.track_offset = 0;
        if (!HostFindArchive()) {
            HostResetCdBackend();
            return 0;
        }
        written = snprintf(g_RageHostDisc.source_path,
                           sizeof(g_RageHostDisc.source_path), "%s",
                           discPath);
        if (written >= 0 &&
            (size_t)written < sizeof(g_RageHostDisc.source_path)) {
            return 1;
        }
        HostResetCdBackend();
        return 0;
    }
    if (DiscPathIsBin(discPath)) {
        int written = snprintf(dataTrackPath, sizeof(dataTrackPath), "%s",
                               discPath);
        if (written < 0 || (size_t)written >= sizeof(dataTrackPath)) {
            return 0;
        }
        g_RageHostDisc.track_offset = 0;
        return HostOpenDataTrack(dataTrackPath);
    }
    if (!DiscPathIsCue(discPath) || Psyz_CdSetDiskPath(discPath) != 0) {
        return 0;
    }
    if (!DiscCueResolveDataTrack(discPath, dataTrackPath,
                                 sizeof(dataTrackPath),
                                 &g_RageHostDisc.track_offset) ||
        !HostOpenDataTrack(dataTrackPath)) {
        HostResetCdBackend();
        return 0;
    }
    return 1;
}

static int HostOpenDisc(const char *discPath) {
    if (!HostOpenDiscImage(discPath)) return 0;
    HostAdoptDiscStreamTable();
    return 1;
}

/* Release archives invite players to drop a disc image next to the
 * executable. Try every plausible file: an extracted RAGE.BIN or an audio
 * track may share the extension but is not itself a mountable game disc. */
static int HostTryOpenAdjacentDisc(void *context, const char *path) {
    (void)context;
    return access(path, R_OK) == 0 &&
           HostOpenDisc(path);
}

static int HostOpenAdjacentDisc(char *path, size_t pathSize) {
    char directory[PATH_MAX];

    return PlatformExecutableDirectory(NULL, directory, sizeof(directory)) &&
           DiscDiscoverImage(directory, path, pathSize,
                             HostTryOpenAdjacentDisc, NULL);
}

int HostInitDisc(void) {
    const char *configuredPath = RuntimeConfigGet("disc.image");
    char discPath[PATH_MAX];
    int choose;

    if (configuredPath == NULL || configuredPath[0] == '\0')
        configuredPath = RuntimeConfigGetForced("disc.cue");

    HostCloseDisc();
    HostSeedStreamTable();
    /* The smoke executable characterizes renderer and game state without
     * bundling retail data.  The release executable never sets this flag.
     * A checked-out disc image still has to reach PsyZ, or CD-DA plays
     * nothing during the smoke runs. */
    if (RuntimeConfigEnabled("runtime.test_mode")) {
        const char *testPath = configuredPath;
        if (testPath == NULL || testPath[0] == '\0')
            testPath = "disc/PAL/Rage Racer (Europe).cue";
        if (access(testPath, R_OK) == 0 &&
            HostOpenDisc(testPath)) {
            return 1;
        }
        return 1;
    }
    if (configuredPath != NULL && configuredPath[0] != '\0') {
        return HostOpenDisc(configuredPath);
    }
    /* Remembering the choice means an install that once worked never asks
     * again, so offer a way back to the picker. */
    choose = RuntimeConfigEnabled("disc.choose");
    /* Opening the disc is the test, not whether the cue is still there: its
     * track files move or get deleted on their own, and a cue that no longer
     * resolves has to send the player back to the picker rather than end the
     * session. */
    if (!choose && HostLoadSavedDiscPath(discPath, sizeof(discPath)) &&
        HostOpenDisc(discPath)) return 1;
    if (!choose && HostOpenAdjacentDisc(discPath, sizeof(discPath))) {
        HostSaveDiscPath(discPath);
        return 1;
    }
    if (!HostChooseDisc(discPath, sizeof(discPath)) ||
        !HostOpenDisc(discPath)) return 0;
    HostSaveDiscPath(discPath);
    return 1;
}

/* Write the whole RAGE.BIN out of the mounted disc, so the modding tools can
 * work on a plain file instead of reimplementing ISO and CUE parsing. */
int HostDumpArchive(const char *path) {
    unsigned char buffer[64 * 1024];
    FILE *output;
    unsigned int offset = 0;
    unsigned int remaining;
    int ok = 1;

    if (path == NULL || g_RageHostDisc.archive.size == 0 ||
        g_RageHostDisc.source_path[0] == '\0' ||
        DiscPathsReferToSameFile(g_RageHostDisc.source_path, path)) {
        return 0;
    }
    remaining = g_RageHostDisc.archive.size;
    output = fopen(path, "wb");
    if (output == NULL) return 0;
    while (remaining > 0) {
        unsigned int chunk = remaining < sizeof(buffer)
                                 ? remaining
                                 : (unsigned int)sizeof(buffer);

        if (!HostReadArchive(offset, buffer, chunk) ||
            fwrite(buffer, 1, chunk, output) != chunk) {
            ok = 0;
            break;
        }
        offset += chunk;
        remaining -= chunk;
    }
    if (fclose(output) != 0) ok = 0;
    if (!ok) remove(path);
    return ok;
}

/*
 * The PS1 drive cannot stream CD-DA while reading data; pausing here is what
 * actually stops the prologue music when menu assets load.
 *
 * A movie's soundtrack is not CD-DA. It is XA, interleaved into the movie's own
 * sectors, and the port streams it from a separate handle that reading an asset
 * does not disturb. Pausing it silenced every movie the game loads behind,
 * which is all of them but the opening one.
 */
static void BeginDataRead(void) {
    if (FmvXaStreaming()) return;
    Psyz_CdBeginDataRead();
}

int HostLoadArchiveIndex(void *entries_ptr, int count) {
    unsigned char data[RAGE_ARCHIVE_INDEX_ENTRY_COUNT *
                       RAGE_ARCHIVE_INDEX_ENTRY_SIZE];
    RageArchiveIndexEntry entries[RAGE_ARCHIVE_INDEX_ENTRY_COUNT];

    if (entries_ptr == NULL || count <= 0 ||
        count > RAGE_ARCHIVE_INDEX_ENTRY_COUNT) {
        return 0;
    }
    BeginDataRead();
    if (!HostReadArchive(0, data, sizeof(data)) ||
        !RageArchiveDecodeIndex(data, sizeof(data), entries, (size_t)count)) {
        return 0;
    }
    memcpy(entries_ptr, entries, (size_t)count * sizeof(entries[0]));
    return 1;
}

int HostLoadAsset(unsigned int byte_offset, unsigned int size, void *destination) {
    if (size > INT_MAX) return 0;
    BeginDataRead();
    return HostReadArchive(byte_offset, destination, size)
               ? (int)(size & ~3u)
               : 0;
}
