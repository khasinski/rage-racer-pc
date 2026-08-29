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
#include <SDL3/SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include <io.h>
int _stricmp(const char *lhs, const char *rhs);
int _strnicmp(const char *lhs, const char *rhs, unsigned long long count);
/* WinAPI's OpenEvent macro collides with PSY-Q's OpenEvent function. */
#ifdef OpenEvent
#undef OpenEvent
#endif
#define access _access
#define close _close
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#ifndef R_OK
#define R_OK 4
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#else
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <libapi.h>
#include <psyz.h>
#include <psyz/cd.h>

#include "psyq/cd_types.h"
#include "game/scratchpad.h"
#include "platform_paths.h"
#include "runtime_config.h"
#include "chd_disc.h"

extern CdlLOC *CdIntToPos(int sector, CdlLOC *position);
extern char SsSetReservedVoice(char voices);
extern void SsSeqClose(short sequence);
extern int _snd_ev_flag;
extern void _SsVmFlush(void);

/* Host storage for values which lived in the PS1 scratchpad or were aliases. */
int g_CourseSelectScrollValue;
int g_McConfirmChoice_v;
unsigned char g_RageScratchpad[0x400];
GameScratchpadRenderState g_RageScratchpadState;

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

void UpdateBootLogoScene(void);
void EnterFrontend(void);
void EnterTitleScreen(void);
void UpdateFrontend(void);
void UpdateFmv(void);
void UpdateTitleScreen(void);
void UpdateMainMenuOpen(void);
void UpdateMainMenuInput(void);
void UpdateMainMenuExit(void);
void EnterCourseSelectScreen(void);
void UpdateCourseSelectScreen(void);
void UpdateRankingScreen(void);
void EnterCarSelectScreen(void);
void UpdateCarSelectScreen(void);
void UpdateCustomizeScreen(void);
void UpdateDesignModeScreen(void);
void UpdateTeamLogoScreen(void);
void UpdateLogoSampleScreen(void);
void UpdateTeamNameScreen(void);
void UpdatePaintColorScreen(void);
void UpdateCarShopScreen(void);
void UpdateEngineerShopScreen(void);
void ShopScreenNoOp(void);
int DrawCourseSelectScreen(int step);
int DrawRankingScreen(int step);
int DrawCarSelectScreen(int step);
int DrawCustomizeScreen(int step);
int DrawDesignModeScreen(int step);
int DrawTeamLogoScreen(int step);
int DrawLogoSampleScreen(int step);
int DrawTeamNameScreen(int step);
int DrawPaintColorScreen(int step);
int DrawCarShopScreen(int step);
unsigned DrawEngineerShopScreen(int step);
void InitMenuMode(void);
void ReturnFromClassFmv(void);
void UpdateMenuMode(void);
void EnterRoundScreen(void);
void UpdateRoundScreen(void);
void EnterRaceScene(void);
void UpdateRaceScene(void);
void EnterLostRaceScreen(void);
void UpdateLostRaceScreen(void);
void EnterRaceEndScreen(void);
void UpdateRaceEndScreen(void);
void UpdateReplayScene(void);
void EnterPrizeScreen(void);
void UpdatePrizeMoneyScreen(void);
void EnterRecordEntry(void);
void UpdateRecordEntry(void);
void EnterAttractScene(void);
void UpdateOptionScene(void);
void UpdateOptionMenuFade(void);
void UpdateOptionRootMenu(void);
void UpdateClassRecordMenu(void);
void UpdateClassRecordBrowse(void);
void UpdateSoundOptionMenu(void);
void UpdateSoundSettingAdjust(void);
void UpdateScreenAdjustScreen(void);
void UpdateControllerConfigScreen(void);
void BeginNegconCalibration(void);
void UpdateNegconNeutralScreen(void);
void UpdateNegconSteerPlayScreen(void);
void UpdateNegconMaxTwistScreen(void);
void EnterMemoryCardMenu(void);
void EnterMemoryCardMenuFromLoad(void);
void UpdateMemoryCardMenu(void);
void EnterBgmSelectScreen(void);
void UpdateBgmSelectScene(void);
void EnterAttractDemo(void);
void UpdateAttractDemoScene(void);
void EnterPrologue(void);
void TickPrologueStep(void);
void ReturnFromEndingFmv(void);
void UpdateEndingStill(void);
void UpdatePrologueLoadStep0(void);
void UpdatePrologueLoadStep1(void);
void UpdatePrologueLoadStep2(void);
void UpdatePrologue(void);
void (*g_PrologueSteps[4])(void) = {
    UpdatePrologueLoadStep0,
    UpdatePrologueLoadStep1,
    UpdatePrologueLoadStep2,
    UpdatePrologue,
};
void (*g_NativeGameModeHandlers[13])(void) = {
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
    NULL,
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
void (*g_FrontendDrawHandlers[4])(void) = {
    UpdateTitleScreen,
    UpdateMainMenuOpen,
    UpdateMainMenuInput,
    UpdateMainMenuExit,
};

static s32 DrawShopScreenNoOp(s32 step) {
    (void)step;
    ShopScreenNoOp();
    return 0;
}

/* Retail main.exe tables at 0x80082EB8 and 0x80082EF0.  They contain code
 * addresses, so copying their 32-bit words into native storage is invalid. */
void (*g_MenuScreenUpdate[14])(void) = {
    EnterCourseSelectScreen,
    UpdateCourseSelectScreen,
    UpdateRankingScreen,
    EnterCarSelectScreen,
    UpdateCarSelectScreen,
    UpdateCustomizeScreen,
    UpdateDesignModeScreen,
    UpdateTeamLogoScreen,
    UpdateLogoSampleScreen,
    UpdateTeamNameScreen,
    UpdatePaintColorScreen,
    UpdateCarShopScreen,
    UpdateEngineerShopScreen,
    ShopScreenNoOp,
};
s32 (*g_MenuScreenDraw[14])(s32) = {
    DrawShopScreenNoOp,
    DrawCourseSelectScreen,
    DrawRankingScreen,
    DrawShopScreenNoOp,
    DrawCarSelectScreen,
    DrawCustomizeScreen,
    DrawDesignModeScreen,
    DrawTeamLogoScreen,
    DrawLogoSampleScreen,
    DrawTeamNameScreen,
    DrawPaintColorScreen,
    DrawCarShopScreen,
    (s32 (*)(s32))DrawEngineerShopScreen,
    DrawShopScreenNoOp,
};

int MapPs1Scratchpad(void) {
    memset(g_RageScratchpad, 0, sizeof(g_RageScratchpad));
    memset(&g_RageScratchpadState, 0, sizeof(g_RageScratchpadState));
    return 1;
}

enum { RAGE_CD_SECTOR_SIZE = 2352, RAGE_ISO_SECTOR_SIZE = 2048 };

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

static int HostPathEndsWithCue(const char *path) {
    size_t length = strlen(path);
    return length > 4 && strcasecmp(path + length - 4, ".cue") == 0;
}

static int HostPathEndsWithChd(const char *path) {
    size_t length = strlen(path);
    return length > 4 && strcasecmp(path + length - 4, ".chd") == 0;
}

static int HostPathEndsWithBin(const char *path) {
    size_t length = strlen(path);
    return length > 4 && strcasecmp(path + length - 4, ".bin") == 0;
}

static int HostPathEndsWithDisc(const char *path) {
    return HostPathEndsWithCue(path) || HostPathEndsWithBin(path) ||
           HostPathEndsWithChd(path);
}

static int HostReadTextFile(const char *path, char *value, size_t size) {
    FILE *file = fopen(path, "r");
    if (file == NULL || fgets(value, (int)size, file) == NULL) {
        if (file != NULL) fclose(file);
        return 0;
    }
    fclose(file);
    value[strcspn(value, "\r\n")] = '\0';
    return value[0] != '\0';
}

/* A disc image dropped next to the executable is the layout the release
 * archives invite, and the one players reach for first. */
static int HostFindAdjacentCue(char *cue, size_t size) {
    char directory[PATH_MAX];
    if (!PlatformExecutableDirectory(NULL, directory, sizeof(directory)))
        return 0;
#ifdef _WIN32
    {
        char pattern[PATH_MAX];
        WIN32_FIND_DATAA entry;
        HANDLE search;
        if (snprintf(pattern, sizeof(pattern), "%s\\*.*", directory) >=
            (int)sizeof(pattern)) return 0;
        search = FindFirstFileA(pattern, &entry);
        if (search == INVALID_HANDLE_VALUE) return 0;
        do {
            if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!HostPathEndsWithDisc(entry.cFileName)) continue;
            if (snprintf(cue, size, "%s\\%s", directory, entry.cFileName) <
                    (int)size && access(cue, R_OK) == 0) {
                FindClose(search);
                return 1;
            }
        } while (FindNextFileA(search, &entry));
        FindClose(search);
    }
#else
    {
        DIR *handle = opendir(directory);
        struct dirent *entry;
        if (handle == NULL) return 0;
        while ((entry = readdir(handle)) != NULL) {
            if (!HostPathEndsWithDisc(entry->d_name)) continue;
            if (snprintf(cue, size, "%s/%s", directory, entry->d_name) <
                    (int)size && access(cue, R_OK) == 0) {
                closedir(handle);
                return 1;
            }
        }
        closedir(handle);
    }
#endif
    cue[0] = '\0';
    return 0;
}

static void HostMakeDiscConfigPath(char *path, size_t size) {
    if (!PlatformUserConfigPath("disc-cue-path", path, size))
        snprintf(path, size, "%s", "disc-cue-path");
}

static void HostSaveDiscCue(const char *cue) {
    char directory[PATH_MAX];
    char path[PATH_MAX];
    FILE *file;

    if (!PlatformUserConfigDirectory(directory, sizeof(directory)) ||
        !PlatformEnsureDirectory(directory)) return;
    HostMakeDiscConfigPath(path, sizeof(path));
    file = fopen(path, "w");
    if (file == NULL) return;
    fputs(cue, file);
    fputc('\n', file);
    fclose(file);
}

typedef struct RageHostDiscDialog {
    SDL_AtomicInt completed;
    char *path;
    size_t pathSize;
    int accepted;
} RageHostDiscDialog;

static void SDLCALL HostDiscDialogComplete(
    void *userdata, const char *const *files, int filter) {
    RageHostDiscDialog *dialog = userdata;
    (void)filter;
    if (files != NULL && files[0] != NULL &&
        snprintf(dialog->path, dialog->pathSize, "%s", files[0]) <
            (int)dialog->pathSize)
        dialog->accepted = 1;
    SDL_SetAtomicInt(&dialog->completed, 1);
}

static int HostChooseDisc(char *cue, size_t size) {
    static const SDL_DialogFileFilter filters[] = {
        {"Rage Racer disc images", "cue;bin;chd"},
        {"All files", "*"},
    };
    RageHostDiscDialog dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.path = cue;
    dialog.pathSize = size;
    cue[0] = '\0';
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-port: cannot initialize disc picker: %s\n",
                SDL_GetError());
        return 0;
    }
    SDL_ShowOpenFileDialog(HostDiscDialogComplete, &dialog, NULL, filters,
                           (int)(sizeof(filters) / sizeof(filters[0])), NULL,
                           false);
    while (!SDL_GetAtomicInt(&dialog.completed)) {
        SDL_PumpEvents();
        SDL_Delay(10);
    }
    if (!dialog.accepted && SDL_GetError()[0] != '\0')
        fprintf(stderr, "rage-port: disc picker failed: %s\n", SDL_GetError());
    return dialog.accepted && HostPathEndsWithDisc(cue) &&
           access(cue, R_OK) == 0;
}

static unsigned int HostLe32(const unsigned char *value) {
    return (unsigned int)value[0] | ((unsigned int)value[1] << 8)
         | ((unsigned int)value[2] << 16) | ((unsigned int)value[3] << 24);
}

static int HostParseCue(const char *cue, char *image, size_t image_size,
                            long *track_offset) {
    FILE *file = fopen(cue, "r");
    char line[PATH_MAX + 64];
    char cue_directory[PATH_MAX];
    char image_name[PATH_MAX] = {0};
    char *slash;
    char *backslash;
    int data_track_seen = 0;

    if (file == NULL) return 0;
    snprintf(cue_directory, sizeof(cue_directory), "%s", cue);
    slash = strrchr(cue_directory, '/');
    backslash = strrchr(cue_directory, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) {
        slash = backslash;
    }
    if (slash != NULL) *slash = '\0'; else snprintf(cue_directory, sizeof(cue_directory), ".");
    *track_offset = 0;
    while (fgets(line, sizeof(line), file)) {
        char *quote;
        if (strncasecmp(line, "FILE", 4) == 0) {
            quote = strchr(line, '\"');
            if (quote != NULL) {
                char *end = strchr(quote + 1, '\"');
                if (end != NULL) {
                    *end = '\0';
                    snprintf(image_name, sizeof(image_name), "%s", quote + 1);
                }
            }
        } else if (strncasecmp(line, "  TRACK", 7) == 0 || strncasecmp(line, "TRACK", 5) == 0) {
            data_track_seen = strstr(line, "MODE1/2352") != NULL || strstr(line, "MODE2/2352") != NULL;
        } else if (data_track_seen && (strncasecmp(line, "    INDEX 01", 12) == 0 || strncasecmp(line, "INDEX 01", 8) == 0)) {
            int minute, second, frame;
            if (sscanf(line, "%*s %*s %d:%d:%d", &minute, &second, &frame) == 3)
                *track_offset = (long)(minute * 60 * 75 + second * 75 + frame) * RAGE_CD_SECTOR_SIZE;
            break;
        }
    }
    fclose(file);
    if (image_name[0] == '\0') return 0;
    if (image_name[0] == '/' ||
        (isalpha((unsigned char)image_name[0]) && image_name[1] == ':')) {
        snprintf(image, image_size, "%s", image_name);
    }
    else {
        size_t directory_length = strlen(cue_directory);
        size_t name_length = strlen(image_name);
        if (directory_length + 1 + name_length + 1 > image_size) return 0;
        memcpy(image, cue_directory, directory_length);
        image[directory_length] = '/';
        memcpy(image + directory_length + 1, image_name, name_length + 1);
    }
    return access(image, R_OK) == 0;
}

static int HostReadSector(long sector, unsigned char *buffer) {
    if (g_RageHostDisc.chd) {
        unsigned char raw[RAGE_CD_SECTOR_SIZE];
        if (!ChdReadRawSector((unsigned int)sector, raw)) return 0;
        memcpy(buffer, raw + g_RageHostDisc.user_offset,
               RAGE_ISO_SECTOR_SIZE);
        return 1;
    }
    if (g_RageHostDisc.file == NULL
        || fseek(g_RageHostDisc.file, g_RageHostDisc.track_offset
                 + sector * RAGE_CD_SECTOR_SIZE + g_RageHostDisc.user_offset, SEEK_SET) != 0)
        return 0;
    return fread(buffer, 1, RAGE_ISO_SECTOR_SIZE, g_RageHostDisc.file) == RAGE_ISO_SECTOR_SIZE;
}

static int HostFindArchive(void) {
    unsigned char sector[RAGE_ISO_SECTOR_SIZE];
    unsigned int root_sector;
    unsigned int root_size;
    unsigned int offset;
    int user_offset;

    for (user_offset = 16; user_offset <= 24; user_offset += 8) {
        g_RageHostDisc.user_offset = user_offset;
        if (HostReadSector(16, sector) && memcmp(&sector[1], "CD001", 5) == 0) break;
    }
    if (user_offset > 24 || sector[156] < 34) return 0;
    root_sector = HostLe32(&sector[158]);
    root_size = HostLe32(&sector[166]);
    for (offset = 0; offset < root_size; offset += RAGE_ISO_SECTOR_SIZE) {
        unsigned int cursor = 0;
        if (!HostReadSector(root_sector + offset / RAGE_ISO_SECTOR_SIZE, sector)) return 0;
        while (cursor < RAGE_ISO_SECTOR_SIZE) {
            unsigned int length = sector[cursor];
            unsigned int name_length;
            const unsigned char *record;
            if (length == 0) break;
            if (length < 34 || cursor + length > RAGE_ISO_SECTOR_SIZE) return 0;
            record = &sector[cursor];
            name_length = record[32];
            if (name_length >= 8 && strncasecmp((const char *)&record[33], "RAGE.BIN", 8) == 0
                && (name_length == 8 || record[41] == ';')) {
                g_RageHostDisc.archive_sector = HostLe32(&record[2]);
                g_RageHostDisc.archive_size = HostLe32(&record[10]);
            } else if (name_length >= 8 &&
                       strncasecmp((const char *)&record[33], "RAGE.STR", 8) == 0 &&
                       (name_length == 8 || record[41] == ';')) {
                g_RageHostDisc.stream_sector = HostLe32(&record[2]);
                g_RageHostDisc.stream_size = HostLe32(&record[10]);
            }
            cursor += length;
        }
    }
    return g_RageHostDisc.archive_size > 0 && g_RageHostDisc.stream_size > 0;
}

int HostReadStreamSector(unsigned int sector, unsigned char *raw) {
    long offset;
    if (raw == NULL || (!g_RageHostDisc.chd && g_RageHostDisc.file == NULL) ||
        sector >= (unsigned long)((g_RageHostDisc.stream_size + 2047) / 2048)) {
        return 0;
    }
    if (g_RageHostDisc.chd)
        return ChdReadRawSector(
            (unsigned int)(g_RageHostDisc.stream_sector + (long)sector), raw);
    offset = g_RageHostDisc.track_offset +
             (g_RageHostDisc.stream_sector + (long)sector) * RAGE_CD_SECTOR_SIZE;
    if (fseek(g_RageHostDisc.file, offset, SEEK_SET) != 0) return 0;
    return fread(raw, 1, RAGE_CD_SECTOR_SIZE, g_RageHostDisc.file) ==
           RAGE_CD_SECTOR_SIZE;
}

int HostStreamAbsoluteSector(unsigned int sector) {
    if (g_RageHostDisc.stream_size <= 0) return -1;
    return (int)(g_RageHostDisc.stream_sector + sector);
}

static int HostReadArchive(unsigned int offset, void *destination, unsigned int size) {
    unsigned char sector[RAGE_ISO_SECTOR_SIZE];
    unsigned char *output = destination;
    FILE *test_archive;

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
    if ((long)offset + size > g_RageHostDisc.archive_size) return 0;
    while (size > 0) {
        unsigned int sector_offset = offset % RAGE_ISO_SECTOR_SIZE;
        unsigned int chunk = RAGE_ISO_SECTOR_SIZE - sector_offset;
        if (chunk > size) chunk = size;
        if (!HostReadSector(g_RageHostDisc.archive_sector + offset / RAGE_ISO_SECTOR_SIZE, sector)) return 0;
        memcpy(output, sector + sector_offset, chunk);
        output += chunk;
        offset += chunk;
        size -= chunk;
    }
    return 1;
}

/* Resolves a cue all the way to a readable archive, so a path that no longer
 * works is reported as such instead of failing later on. */
static int HostOpenDisc(const char *cue, char *image, size_t image_size) {
    if (HostPathEndsWithChd(cue)) {
        if (!ChdOpen(cue)) return 0;
        g_RageHostDisc.chd = 1;
        g_RageHostDisc.track_offset = 0;
        if (!HostFindArchive()) {
            ChdClose();
            g_RageHostDisc.chd = 0;
            return 0;
        }
        return 1;
    }
    if (HostPathEndsWithBin(cue)) {
        if (snprintf(image, image_size, "%s", cue) >= (int)image_size)
            return 0;
        g_RageHostDisc.track_offset = 0;
        g_RageHostDisc.file = fopen(image, "rb");
        if (g_RageHostDisc.file == NULL || !HostFindArchive()) {
            if (g_RageHostDisc.file != NULL) fclose(g_RageHostDisc.file);
            g_RageHostDisc.file = NULL;
            return 0;
        }
        return 1;
    }
    if (!HostPathEndsWithCue(cue) || Psyz_CdSetDiskPath(cue) != 0 ||
        !HostParseCue(cue, image, image_size,
                          &g_RageHostDisc.track_offset)) return 0;
    g_RageHostDisc.file = fopen(image, "rb");
    if (g_RageHostDisc.file == NULL || !HostFindArchive()) {
        if (g_RageHostDisc.file != NULL) fclose(g_RageHostDisc.file);
        g_RageHostDisc.file = NULL;
        return 0;
    }
    return 1;
}

int HostInitDisc(void) {
    const char *environment_cue = RuntimeConfigGet("disc.image");
    char cue[PATH_MAX];
    char image[PATH_MAX];
    char config_path[PATH_MAX];
    int choose;

    if (environment_cue == NULL || environment_cue[0] == '\0')
        environment_cue = RuntimeConfigGetForced("disc.cue");

    ChdClose();
    memset(&g_RageHostDisc, 0, sizeof(g_RageHostDisc));
    /* The smoke executable characterizes renderer and game state without
     * bundling retail data.  The release executable never sets this flag.
     * A checked-out disc image still has to reach PsyZ, or CD-DA plays
     * nothing during the smoke runs. */
    if (RuntimeConfigEnabled("runtime.test_mode")) {
        const char *test_cue = environment_cue;
        if (test_cue == NULL || test_cue[0] == '\0')
            test_cue = "disc/PAL/Rage Racer (Europe).cue";
        if (access(test_cue, R_OK) == 0 &&
            HostOpenDisc(test_cue, image, sizeof(image))) return 1;
        return 1;
    }
    if (environment_cue != NULL && environment_cue[0] != '\0') {
        snprintf(cue, sizeof(cue), "%s", environment_cue);
        return HostOpenDisc(cue, image, sizeof(image));
    }
    /* Remembering the choice means an install that once worked never asks
     * again, so offer a way back to the picker. */
    choose = RuntimeConfigEnabled("disc.choose");
    HostMakeDiscConfigPath(config_path, sizeof(config_path));
    /* Opening the disc is the test, not whether the cue is still there: its
     * track files move or get deleted on their own, and a cue that no longer
     * resolves has to send the player back to the picker rather than end the
     * session. */
    if (!choose && HostReadTextFile(config_path, cue, sizeof(cue)) &&
        HostOpenDisc(cue, image, sizeof(image))) return 1;
    if (!choose && HostFindAdjacentCue(cue, sizeof(cue)) &&
        HostOpenDisc(cue, image, sizeof(image))) {
        HostSaveDiscCue(cue);
        return 1;
    }
    if (!HostChooseDisc(cue, sizeof(cue)) ||
        !HostOpenDisc(cue, image, sizeof(image))) return 0;
    HostSaveDiscCue(cue);
    return 1;
}

typedef struct RageHostCdEntry {
    uint32_t byte_offset;
    uint32_t size;
} RageHostCdEntry;

/* Write the whole RAGE.BIN out of the mounted disc, so the modding tools can
 * work on a plain file instead of reimplementing ISO and CUE parsing. */
int HostDumpArchive(const char *path) {
    unsigned char *buffer;
    FILE *output;
    long size = g_RageHostDisc.archive_size;
    int ok;

    if (size <= 0) return 0;
    buffer = malloc((size_t)size);
    if (buffer == NULL) return 0;
    if (!HostReadArchive(0, buffer, (unsigned int)size)) {
        free(buffer);
        return 0;
    }
    output = fopen(path, "wb");
    if (output == NULL) {
        free(buffer);
        return 0;
    }
    ok = fwrite(buffer, 1, (size_t)size, output) == (size_t)size;
    fclose(output);
    free(buffer);
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
int FmvXaStreaming(void);

static void BeginDataRead(void) {
    if (FmvXaStreaming()) return;
    Psyz_CdBeginDataRead();
}

int HostLoadArchiveIndex(void *entries_ptr, int count) {
    RageHostCdEntry *entries = entries_ptr;
    uint32_t words[270];
    int index;
    BeginDataRead();
    if (count > 135 || !HostReadArchive(0, words, sizeof(words))) return 0;
    for (index = 0; index < count; index++) {
        entries[index].byte_offset = words[index * 2] * 2048u;
        entries[index].size = words[index * 2 + 1];
    }
    return 1;
}

int HostLoadAsset(unsigned int byte_offset, unsigned int size, void *destination) {
    BeginDataRead();
    return HostReadArchive(byte_offset, destination, size) ? (int)(size & ~3u) : 0;
}

void ApplyMatrixLV(void *matrix, const int32_t *input, int32_t *output) {
    const int16_t *m = matrix;
    int row;
    for (row = 0; row < 3; row++) {
        int64_t value = (int64_t)m[row * 3] * input[0]
                      + (int64_t)m[row * 3 + 1] * input[1]
                      + (int64_t)m[row * 3 + 2] * input[2];
        output[row] = (int32_t)(value >> 12);
    }
}

void MatrixApplyVectorComponents(MATRIX *matrix, int32_t x, int32_t y, int32_t z,
                                 int32_t *out_x, int32_t *out_y, int32_t *out_z) {
    int32_t input[3] = {x, y, z};
    int32_t output[3];
    ApplyMatrixLV(matrix, input, output);
    *out_x = output[0];
    *out_y = output[1];
    *out_z = output[2];
}

void MatrixApplyZRotation(MATRIX *matrix, int32_t degrees) {
    MATRIX rotation = {0};
    int angle;
    if (degrees == 0) return;
    angle = degrees / 360;
    rotation.m[0][0] = (short)rcos(angle);
    rotation.m[0][1] = (short)-rsin(angle);
    rotation.m[1][0] = (short)rsin(angle);
    rotation.m[1][1] = (short)rcos(angle);
    rotation.m[2][2] = 0x1000;
    MulMatrix(matrix, &rotation);
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

void TransformCollisionVector(const int16_t *input, int32_t *output) {
    SVECTOR vector;
    VECTOR transformed;

    vector.vx = input[0];
    vector.vy = input[1];
    vector.vz = input[2];
    vector.pad = 0;
    ApplyRotMatrix(&vector, &transformed);
    output[0] = transformed.vx;
    output[1] = transformed.vy;
    output[2] = transformed.vz;
}

/* The following adapters keep optional PS1 services non-fatal on the host.
 * Their real filesystem/audio implementations are introduced before those
 * subsystems are enabled in the native startup path. */
#define VOID_ADAPTER(name) void name(void) {}
#define ZERO_ADAPTER(name) long name(void) { return 0; }
#define FAIL_ADAPTER(name) long name(void) { return -1; }

void BiosBuInit(void) { _bu_init(); }
VOID_ADAPTER(BiosExit)
VOID_ADAPTER(BiosSetMemSize)
VOID_ADAPTER(KernelCallbackSlot3)
VOID_ADAPTER(SetDMAInterruptState)
VOID_ADAPTER(StCdInterrupt)
VOID_ADAPTER(StUnSetRing)
long BiosFileOpen(void *path, long mode) {
    if (path == NULL) return -1;
    return psyz_open(path, (int)mode);
}

long BiosFileSeek(long fd, long offset, long whence) {
    return lseek((int)fd, offset, (int)whence);
}

long BiosFileRead(long fd, void *buffer, long length) {
    return read((int)fd, buffer, (size_t)length);
}

long BiosFileWrite(long fd, void *buffer, long length) {
    return write((int)fd, buffer, (size_t)length);
}

long BiosFileClose(long fd) { return close((int)fd); }

void *BiosFirstFile(char *path, void *entry) {
    return firstfile(path, entry);
}

void *BiosNextFile(void *entry) { return nextfile(entry); }

long BiosFormatDevice(void *device) { return format(device); }
ZERO_ADAPTER(CdPosToInt_Local)
ZERO_ADAPTER(CdReadBreak)
long HostCdAudioEnded(void) { return Psyz_CdAudioEnded(); }
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
ZERO_ADAPTER(SsSetSpuInputAttr)
unsigned char SsSetVoiceCount(unsigned char voices) {
    return (unsigned char)SsSetReservedVoice((char)voices);
}
ZERO_ADAPTER(SsStartSoundTickMode1)
ZERO_ADAPTER(SsStopSoundTick)
ZERO_ADAPTER(StGetBackloc)
ZERO_ADAPTER(ssinit)

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
