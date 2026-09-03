#include <libetc.h>
#include <libgte.h>
#include <libpress.h>

#include <stdint.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include <io.h>
/* WinAPI turns several of its own calls into macros that take the names of
 * PSY-Q functions. OpenEvent and LoadImage are the two this file reaches. */
#ifdef OpenEvent
#undef OpenEvent
#endif
#ifdef LoadImage
#undef LoadImage
#endif
#define access _access
#define close _close
#ifndef R_OK
#define R_OK 4
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#else
#include <unistd.h>
#endif

#include <libapi.h>
#include <psyz.h>
#include <psyz/cd.h>

#include "psyq/cd_types.h"
#include "game/render_state.h"
#include "game/state.h"
#include "game/menu.h"
#include "game/fmv.h"
#include "game/records_internal.h"
#include "game/replay_internal.h"
#include "game/round_screen_internal.h"
#include "game/screens.h"
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

extern CdlLOC *CdIntToPos(int sector, CdlLOC *position);
extern char SsSetReservedVoice(char voices);
extern void SsSeqClose(short sequence);
extern int _snd_ev_flag;
extern void _SsVmFlush(void);

/* Values the game keeps between calls; on the PS1 these lived in fast RAM. */
GameRenderState g_RenderState;
CarTrackWork g_CarTrackWork;

static char s_RageMemoryCardDirectory[PATH_MAX];

static int HostAdjustStoragePath(char *dst, const char *src, int maxlen) {
    const char *name;
    char card[5];
    int written;

    if (dst == NULL || src == NULL || maxlen <= 0 ||
        strlen(src) < 5 || src[0] != 'b' || src[1] != 'u' ||
        src[4] != ':' ||
        !((src[2] == '0' || src[2] == '1') && src[3] == '0'))
        return -1;

    memcpy(card, src, 4);
    card[4] = '\0';
    name = src + 5;
    if (name[0] == '\0' || name[0] == '*') {
        written = snprintf(dst, (size_t)maxlen, "%s%c%s%c",
                           s_RageMemoryCardDirectory,
#ifdef _WIN32
                           '\\', card, '\\');
#else
                           '/', card, '/');
#endif
    } else {
        written = snprintf(dst, (size_t)maxlen, "%s%c%s%c%s",
                           s_RageMemoryCardDirectory,
#ifdef _WIN32
                           '\\', card, '\\', name);
#else
                           '/', card, '/', name);
#endif
    }
    if (written < 0 || written >= maxlen) {
        dst[0] = '\0';
        return -1;
    }
    return written;
}

int HostInitStorage(void) {
    char cardDirectory[PATH_MAX];
    char executableDirectory[PATH_MAX];
    int card;

    if (!(PlatformExecutableDirectory(
              NULL, executableDirectory, sizeof(executableDirectory)) &&
          PlatformExistingPortableStateDirectory(
              executableDirectory, s_RageMemoryCardDirectory,
              sizeof(s_RageMemoryCardDirectory))) &&
        !PlatformUserStateDirectory(s_RageMemoryCardDirectory,
                                        sizeof(s_RageMemoryCardDirectory)))
        return 0;
    if (!PlatformEnsureDirectory(s_RageMemoryCardDirectory))
        return 0;

    for (card = 0; card < 2; card++) {
        int written = snprintf(cardDirectory, sizeof(cardDirectory), "%s%cbu%d0",
                               s_RageMemoryCardDirectory,
#ifdef _WIN32
                               '\\', card);
#else
                               '/', card);
#endif
        if (written < 0 || (size_t)written >= sizeof(cardDirectory) ||
            !PlatformEnsureDirectory(cardDirectory))
            return 0;
    }
    Psyz_AdjustPathCB(HostAdjustStoragePath);
    return 1;
}

void EnterRaceScene(void);
void UpdateRaceScene(void);
void EnterPrizeScreen(void);
void UpdatePrizeMoneyScreen(void);
void EnterAttractScene(void);
void EnterMemoryCardMenu(void);
void EnterMemoryCardMenuFromLoad(void);
void UpdateMemoryCardMenu(void);
void EnterBgmSelectScreen(void);
void UpdateBgmSelectScene(void);
void EnterAttractDemo(void);
void UpdateAttractDemoScene(void);
void EnterPrologue(void);
void TickPrologueStep(void);
void UpdateEndingStill(void);
void (*g_NativeGameModeHandlers[OPTION_MODE_COUNT])(void) = {
    UpdateOptionMenuFade,
    UpdateOptionRootMenu,
    UpdateClassRecordMenu,
    UpdateClassRecordBrowse,
    UpdateSoundOptionMenu,
    UpdateSoundSettingAdjust,
    UpdateScreenAdjustScreen,
    UpdateControllerConfigScreen,
    BeginNegconCalibration,
    UpdateNegconNeutralScreen,
    UpdateNegconSteerPlayScreen,
    UpdateNegconMaxTwistScreen,
};
void (*g_SceneHandlers[40])(void) = {
    [1] = UpdateBootLogoScene,
    [2] = EnterFrontend,
    [3] = EnterTitleScreen,
    [4] = UpdateFrontend,
    [5] = UpdateFmv,
    [6] = InitMenuMode,
    [7] = ReturnFromClassFmv,
    [8] = UpdateMenuMode,
    [9] = EnterRoundScreen,
    [10] = UpdateRoundScreen,
    [11] = EnterRaceScene,
    [12] = UpdateRaceScene,
    [13] = EnterLostRaceScreen,
    [14] = UpdateLostRaceScreen,
    [15] = EnterRaceEndScreen,
    [16] = UpdateRaceEndScreen,
    [17] = UpdateReplayScene,
    [18] = EnterPrizeScreen,
    [19] = UpdatePrizeMoneyScreen,
    [20] = EnterRecordEntry,
    [21] = UpdateRecordEntry,
    [22] = EnterAttractScene,
    [23] = UpdateOptionScene,
    [24] = EnterMemoryCardMenu,
    [25] = EnterMemoryCardMenuFromLoad,
    [26] = UpdateMemoryCardMenu,
    [27] = EnterBgmSelectScreen,
    [28] = UpdateBgmSelectScene,
    [29] = EnterAttractDemo,
    [30] = UpdateAttractDemoScene,
    [31] = EnterPrologue,
    [32] = TickPrologueStep,
    [33] = ReturnFromEndingFmv,
    [34] = UpdateEndingStill,
};
void (*g_FrontendDrawHandlers[])(void) = {
    UpdateTitleScreen,
    UpdateMainMenuOpen,
    UpdateMainMenuInput,
    UpdateMainMenuExit,
};

static s32 DrawShopScreenNoOp(s32 step) {
    (void)step;
    return 0;
}

static void ShopScreenNoOp(void) {}

/* Retail main.exe tables at 0x80082EB8 and 0x80082EF0.  They contain code
 * addresses, so copying their 32-bit words into native storage is invalid. */
void (*g_MenuScreenUpdate[MENU_SCREEN_COUNT])(void) = {
    [MENU_SCREEN_BOOTSTRAP] = EnterCourseSelectScreen,
    [MENU_SCREEN_COURSE_SELECT] = UpdateCourseSelectScreen,
    [MENU_SCREEN_RANKING] = UpdateRankingScreen,
    [MENU_SCREEN_ENTER_CAR_SELECT] = EnterCarSelectScreen,
    [MENU_SCREEN_CAR_SELECT] = UpdateCarSelectScreen,
    [MENU_SCREEN_CUSTOMIZE] = UpdateCustomizeScreen,
    [MENU_SCREEN_DESIGN_MODE] = UpdateDesignModeScreen,
    [MENU_SCREEN_TEAM_LOGO] = UpdateTeamLogoScreen,
    [MENU_SCREEN_LOGO_SAMPLE] = UpdateLogoSampleScreen,
    [MENU_SCREEN_TEAM_NAME] = UpdateTeamNameScreen,
    [MENU_SCREEN_PAINT_COLOR] = UpdatePaintColorScreen,
    [MENU_SCREEN_CAR_SHOP] = UpdateCarShopScreen,
    [MENU_SCREEN_ENGINEER_SHOP] = UpdateEngineerShopScreen,
    [MENU_SCREEN_UNUSED] = ShopScreenNoOp,
};
s32 (*g_MenuScreenDraw[MENU_SCREEN_COUNT])(s32) = {
    [MENU_SCREEN_BOOTSTRAP] = DrawShopScreenNoOp,
    [MENU_SCREEN_COURSE_SELECT] = DrawCourseSelectScreen,
    [MENU_SCREEN_RANKING] = DrawRankingScreen,
    [MENU_SCREEN_ENTER_CAR_SELECT] = DrawShopScreenNoOp,
    [MENU_SCREEN_CAR_SELECT] = DrawCarSelectScreen,
    [MENU_SCREEN_CUSTOMIZE] = DrawCustomizeScreen,
    [MENU_SCREEN_DESIGN_MODE] = DrawDesignModeScreen,
    [MENU_SCREEN_TEAM_LOGO] = DrawTeamLogoScreen,
    [MENU_SCREEN_LOGO_SAMPLE] = DrawLogoSampleScreen,
    [MENU_SCREEN_TEAM_NAME] = DrawTeamNameScreen,
    [MENU_SCREEN_PAINT_COLOR] = DrawPaintColorScreen,
    [MENU_SCREEN_CAR_SHOP] = DrawCarShopScreen,
    [MENU_SCREEN_ENGINEER_SHOP] = DrawEngineerShopScreen,
    [MENU_SCREEN_UNUSED] = DrawShopScreenNoOp,
};

typedef struct RageHostDisc {
    FILE *file;
    int chd;
    long track_offset;
    long archive_sector;
    long archive_size;
    long stream_sector;
    long stream_size;
    int user_offset;
} RageHostDisc;

static RageHostDisc g_RageHostDisc;

static void HostMakeDiscConfigPath(char *path, size_t size) {
    /* Keep the historical key so existing installations retain their choice. */
    if (!PlatformUserConfigPath("disc-cue-path", path, size))
        snprintf(path, size, "%s", "disc-cue-path");
}

static int HostLoadSavedDiscPath(char *path, size_t size) {
    char configPath[PATH_MAX];

    HostMakeDiscConfigPath(configPath, sizeof(configPath));
    return DiscReadSavedPath(configPath, path, size);
}

static void HostSaveDiscPath(const char *discPath) {
    char directory[PATH_MAX];
    char path[PATH_MAX];
    FILE *file;

    if (!PlatformUserConfigDirectory(directory, sizeof(directory)) ||
        !PlatformEnsureDirectory(directory)) return;
    HostMakeDiscConfigPath(path, sizeof(path));
    file = fopen(path, "w");
    if (file == NULL) return;
    fputs(discPath, file);
    fputc('\n', file);
    fclose(file);
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
    g_RageHostDisc.archive_sector = archive.lba;
    g_RageHostDisc.archive_size = archive.size;
    g_RageHostDisc.stream_sector = stream.lba;
    g_RageHostDisc.stream_size = stream.size;
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
    unsigned long stream_sectors;
    unsigned long absolute_sector;

    stream_sectors = (unsigned long)g_RageHostDisc.stream_size /
                         DISC_ISO_SECTOR_SIZE +
                     ((unsigned long)g_RageHostDisc.stream_size %
                          DISC_ISO_SECTOR_SIZE !=
                      0);
    if (raw == NULL || (!g_RageHostDisc.chd && g_RageHostDisc.file == NULL) ||
        g_RageHostDisc.stream_size <= 0 || sector >= stream_sectors ||
        g_RageHostDisc.stream_sector < 0 ||
        (unsigned long)g_RageHostDisc.stream_sector > ULONG_MAX - sector) {
        return 0;
    }
    absolute_sector = (unsigned long)g_RageHostDisc.stream_sector + sector;
    if (absolute_sector > UINT_MAX) return 0;
    return HostReadRawSector(NULL, (unsigned int)absolute_sector, raw);
}

int HostStreamAbsoluteSector(unsigned int sector) {
    unsigned long absolute;

    if (g_RageHostDisc.stream_size <= 0 ||
        g_RageHostDisc.stream_sector < 0 ||
        (unsigned long)g_RageHostDisc.stream_sector > ULONG_MAX - sector) {
        return -1;
    }
    absolute = (unsigned long)g_RageHostDisc.stream_sector + sector;
    return absolute <= INT_MAX ? (int)absolute : -1;
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
    if (g_RageHostDisc.archive_size < 0 ||
        (unsigned long)offset > (unsigned long)g_RageHostDisc.archive_size ||
        size > (unsigned long)g_RageHostDisc.archive_size - offset) {
        return 0;
    }
    while (size > 0) {
        unsigned int sector_offset = offset % DISC_ISO_SECTOR_SIZE;
        unsigned int chunk = DISC_ISO_SECTOR_SIZE - sector_offset;
        if (chunk > size) chunk = size;
        if (g_RageHostDisc.archive_sector < 0 ||
            (unsigned long)g_RageHostDisc.archive_sector >
                UINT_MAX - offset / DISC_ISO_SECTOR_SIZE ||
            !HostReadUserSector(
                (unsigned int)g_RageHostDisc.archive_sector +
                    offset / DISC_ISO_SECTOR_SIZE,
                sector)) {
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
    g_RageHostDisc.file = fopen(path, "rb");
    if (g_RageHostDisc.file != NULL && HostFindArchive()) return 1;
    if (g_RageHostDisc.file != NULL) fclose(g_RageHostDisc.file);
    g_RageHostDisc.file = NULL;
    return 0;
}

/* Resolves a selected image all the way to a readable archive, so a path that
 * no longer works is reported before the game starts loading assets. */
static int HostOpenDiscImage(const char *discPath, char *dataTrackPath,
                             size_t dataTrackPathSize) {
    if (DiscPathIsChd(discPath)) {
        if (!ChdOpen(discPath)) return 0;
        g_RageHostDisc.chd = 1;
        g_RageHostDisc.track_offset = 0;
        if (!HostFindArchive()) {
            ChdClose();
            g_RageHostDisc.chd = 0;
            return 0;
        }
        return 1;
    }
    if (DiscPathIsBin(discPath)) {
        int written = snprintf(dataTrackPath, dataTrackPathSize, "%s",
                               discPath);
        if (written < 0 || (size_t)written >= dataTrackPathSize) {
            return 0;
        }
        g_RageHostDisc.track_offset = 0;
        return HostOpenDataTrack(dataTrackPath);
    }
    if (!DiscPathIsCue(discPath) || Psyz_CdSetDiskPath(discPath) != 0 ||
        !DiscCueResolveDataTrack(discPath, dataTrackPath, dataTrackPathSize,
                                 &g_RageHostDisc.track_offset)) {
        return 0;
    }
    return HostOpenDataTrack(dataTrackPath);
}

static int HostOpenDisc(const char *discPath, char *dataTrackPath,
                        size_t dataTrackPathSize) {
    if (!HostOpenDiscImage(discPath, dataTrackPath, dataTrackPathSize)) return 0;
    HostAdoptDiscStreamTable();
    return 1;
}

/* Release archives invite players to drop a disc image next to the
 * executable. Try every plausible file: an extracted RAGE.BIN or an audio
 * track may share the extension but is not itself a mountable game disc. */
typedef struct AdjacentDiscContext {
    char *dataTrackPath;
    size_t dataTrackPathSize;
} AdjacentDiscContext;

static int HostTryOpenAdjacentDisc(void *context, const char *path) {
    AdjacentDiscContext *disc = context;
    return access(path, R_OK) == 0 &&
           HostOpenDisc(path, disc->dataTrackPath, disc->dataTrackPathSize);
}

static int HostOpenAdjacentDisc(char *path, size_t pathSize,
                                char *dataTrackPath,
                                size_t dataTrackPathSize) {
    char directory[PATH_MAX];
    AdjacentDiscContext context = {dataTrackPath, dataTrackPathSize};

    return PlatformExecutableDirectory(NULL, directory, sizeof(directory)) &&
           DiscDiscoverImage(directory, path, pathSize,
                             HostTryOpenAdjacentDisc, &context);
}

int HostInitDisc(void) {
    const char *configuredPath = RuntimeConfigGet("disc.image");
    char discPath[PATH_MAX];
    char dataTrackPath[PATH_MAX];
    int choose;

    if (configuredPath == NULL || configuredPath[0] == '\0')
        configuredPath = RuntimeConfigGetForced("disc.cue");

    ChdClose();
    HostSeedStreamTable();
    memset(&g_RageHostDisc, 0, sizeof(g_RageHostDisc));
    /* The smoke executable characterizes renderer and game state without
     * bundling retail data.  The release executable never sets this flag.
     * A checked-out disc image still has to reach PsyZ, or CD-DA plays
     * nothing during the smoke runs. */
    if (RuntimeConfigEnabled("runtime.test_mode")) {
        const char *testPath = configuredPath;
        if (testPath == NULL || testPath[0] == '\0')
            testPath = "disc/PAL/Rage Racer (Europe).cue";
        if (access(testPath, R_OK) == 0 &&
            HostOpenDisc(testPath, dataTrackPath, sizeof(dataTrackPath))) {
            return 1;
        }
        return 1;
    }
    if (configuredPath != NULL && configuredPath[0] != '\0') {
        snprintf(discPath, sizeof(discPath), "%s", configuredPath);
        return HostOpenDisc(discPath, dataTrackPath, sizeof(dataTrackPath));
    }
    /* Remembering the choice means an install that once worked never asks
     * again, so offer a way back to the picker. */
    choose = RuntimeConfigEnabled("disc.choose");
    /* Opening the disc is the test, not whether the cue is still there: its
     * track files move or get deleted on their own, and a cue that no longer
     * resolves has to send the player back to the picker rather than end the
     * session. */
    if (!choose && HostLoadSavedDiscPath(discPath, sizeof(discPath)) &&
        HostOpenDisc(discPath, dataTrackPath, sizeof(dataTrackPath))) return 1;
    if (!choose && HostOpenAdjacentDisc(
                       discPath, sizeof(discPath), dataTrackPath,
                       sizeof(dataTrackPath))) {
        HostSaveDiscPath(discPath);
        return 1;
    }
    if (!HostChooseDisc(discPath, sizeof(discPath)) ||
        !HostOpenDisc(discPath, dataTrackPath, sizeof(dataTrackPath))) return 0;
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

    if (path == NULL || g_RageHostDisc.archive_size <= 0 ||
        (unsigned long)g_RageHostDisc.archive_size > UINT_MAX) {
        return 0;
    }
    remaining = (unsigned int)g_RageHostDisc.archive_size;
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

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output) {
    int row, column, inner;
    MATRIX result = *output;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            int64_t value = 0;
            for (inner = 0; inner < 3; inner++)
                value += (int64_t)left->m[row][inner] * right->m[inner][column];
            result.m[row][column] = (short)(value >> 12);
        }
    }
    *output = result;
    return output;
}

void BiosBuInit(void) { _bu_init(); }
void BiosSetMemSize(s32 megabytes) { (void)megabytes; }
void KernelCallbackSlot3(void) {}
long SetDMAInterruptState(long value) {
    (void)value;
    return 0;
}
long BiosFileOpen(void *path, long mode) {
    if (path == NULL) return -1;
    return psyz_open(path, (int)mode);
}

long BiosFileSeek(long fd, long offset, long whence) {
    return lseek((int)fd, offset, (int)whence);
}

long BiosFileRead(long fd, void *buffer, long length) {
    if (length < 0) return -1;
    return read((int)fd, buffer, (size_t)length);
}

long BiosFileWrite(long fd, void *buffer, long length) {
    if (length < 0) return -1;
    return write((int)fd, buffer, (size_t)length);
}

long BiosFileClose(long fd) { return close((int)fd); }

void *BiosFirstFile(char *path, void *entry) {
    return firstfile(path, entry);
}

void *BiosNextFile(void *entry) { return nextfile(entry); }

long BiosFormatDevice(void *device) { return format(device); }
s32 HostCdAudioEnded(void) { return Psyz_CdAudioEnded(); }
void InitPad(void *buf0, int len0, void *buf1, int len1) {
    (void)InitPAD((char *)buf0, len0, (char *)buf1, len1);
}
void SpuVmDamperStep(void) {
    if (_snd_ev_flag != 1) {
        _snd_ev_flag = 1;
        _SsVmFlush();
        _snd_ev_flag = 0;
    }
}
void SsSeqCloseWrapper(short sequence) { SsSeqClose(sequence); }
void SsSetSpuInputAttr(unsigned char source, unsigned char field,
                       unsigned char value) {
    (void)source;
    (void)field;
    (void)value;
}
unsigned char SsSetVoiceCount(unsigned char voices) {
    return (unsigned char)SsSetReservedVoice((char)voices);
}
void ssinit(void) {}

CdlFILE *DsSearchFile(CdlFILE *file, char *name) {
    const char *marker;
    int track;
    int sector;

    if (file == NULL || name == NULL) return NULL;
    marker = name;
    while ((marker = strstr(marker, "DA")) != NULL) {
        if (marker[2] >= '0' && marker[2] <= '9' &&
            marker[3] >= '0' && marker[3] <= '9') break;
        marker += 2;
    }
    if (marker == NULL) return NULL;
    track = (marker[2] - '0') * 10 + marker[3] - '0';
    sector = Psyz_CdGetTrackSector(track);
    if (sector < 0) return NULL;
    CdIntToPos(sector, &file->pos);
    file->size = 0;
    strncpy(file->name, marker, sizeof(file->name) - 1);
    file->name[sizeof(file->name) - 1] = '\0';
    return file;
}
