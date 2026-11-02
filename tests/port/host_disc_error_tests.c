#include "host_disc.h"
#include "chd_disc.h"
#include "disc_iso.h"
#include "disc_picker.h"
#include "disc_stream_table.h"
#include "fmv_audio.h"
#include "platform_paths.h"
#include "runtime_config.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum {
    TEST_SECTOR_COUNT = 22,
    ROOT_SECTOR = 17,
    RAGE_BIN_SECTOR = 18,
    RAGE_STR_SECTOR = 20,
};

typedef struct FakePickerState {
    const char *paths[4];
    int results[4];
    int step;
    int totalSteps;
    int dialogCalls;
    char lastDialogTitle[256];
    char lastDialogMessage[1024];
} FakePickerState;

static FakePickerState g_PickerState;

int ChdOpen(const char *path) {
    (void)path;
    return 0;
}

void ChdClose(void) {}

int ChdReadRawSector(unsigned int lba, unsigned char *buffer) {
    (void)lba;
    (void)buffer;
    return 0;
}

void DiscStreamTablePublish(const DiscStreamTable *table) { (void)table; }

int FmvXaStreaming(void) { return 0; }

int HostShowDiscPicker(char *path, size_t size) {
    int index = g_PickerState.step;

    if (path == NULL || size == 0 || index >= g_PickerState.totalSteps) {
        return -1;
    }
    g_PickerState.step++;
    if (g_PickerState.results[index] <= 0) {
        path[0] = '\0';
        return g_PickerState.results[index];
    }
    snprintf(path, size, "%s",
             g_PickerState.paths[index] != NULL ? g_PickerState.paths[index]
                                                : "");
    return 1;
}

void HostShowErrorDialog(const char *title, const char *message) {
    g_PickerState.dialogCalls++;
    snprintf(g_PickerState.lastDialogTitle,
             sizeof(g_PickerState.lastDialogTitle), "%s",
             title != NULL ? title : "");
    snprintf(g_PickerState.lastDialogMessage,
             sizeof(g_PickerState.lastDialogMessage), "%s",
             message != NULL ? message : "");
}

static void WriteLe32(unsigned char *data, unsigned int value) {
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
    data[2] = (unsigned char)(value >> 16);
    data[3] = (unsigned char)(value >> 24);
}

static unsigned int WriteDirEntry(unsigned char *record, const char *name,
                                  unsigned int lba, unsigned int size) {
    size_t nameLen = strlen(name);
    unsigned int recordLen = (unsigned int)(33 + nameLen);

    if (recordLen & 1u) recordLen++;
    memset(record, 0, recordLen);
    record[0] = (unsigned char)recordLen;
    WriteLe32(record + 2, lba);
    WriteLe32(record + 10, size);
    record[32] = (unsigned char)nameLen;
    memcpy(record + 33, name, nameLen);
    return recordLen;
}

static int WriteDiscBin(const char *path, int includeRageFiles,
                        size_t trailingBytes) {
    unsigned char sectors[TEST_SECTOR_COUNT][DISC_RAW_SECTOR_SIZE];
    unsigned char *volume;
    unsigned char *root;
    unsigned int offset = 0;
    FILE *file;

    memset(sectors, 0, sizeof(sectors));
    volume = sectors[16] + DISC_MODE2_USER_OFFSET;
    volume[0] = 1;
    memcpy(volume + 1, "CD001", 5);
    volume[156] = 34;
    WriteLe32(volume + 158, ROOT_SECTOR);
    WriteLe32(volume + 166, DISC_ISO_SECTOR_SIZE);

    root = sectors[ROOT_SECTOR] + DISC_MODE2_USER_OFFSET;
    if (includeRageFiles) {
        offset += WriteDirEntry(root + offset, "RAGE.BIN;1", RAGE_BIN_SECTOR,
                                DISC_ISO_SECTOR_SIZE);
        offset += WriteDirEntry(root + offset, "RAGE.STR;1", RAGE_STR_SECTOR,
                                DISC_ISO_SECTOR_SIZE);
    } else {
        offset += WriteDirEntry(root + offset, "RIDGE.BIN;1", RAGE_BIN_SECTOR,
                                DISC_ISO_SECTOR_SIZE);
    }
    file = fopen(path, "wb");
    if (file == NULL) return 0;
    if (fwrite(sectors, 1, sizeof(sectors), file) != sizeof(sectors)) {
        fclose(file);
        return 0;
    }
    if (trailingBytes > 0) {
        unsigned char extra[64] = {0};
        if (trailingBytes > sizeof(extra) ||
            fwrite(extra, 1, trailingBytes, file) != trailingBytes) {
            fclose(file);
            return 0;
        }
    }
    return fclose(file) == 0;
}

static int WriteTextFile(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");

    if (file == NULL) return 0;
    if (fputs(text, file) < 0) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static int MakeTemporaryDirectory(char *path, size_t size) {
#ifdef _WIN32
    char base[MAX_PATH];
    char temporary[MAX_PATH];
    if (GetTempPathA(sizeof(base), base) == 0 ||
        GetTempFileNameA(base, "rhd", 0, temporary) == 0 ||
        !DeleteFileA(temporary) || !CreateDirectoryA(temporary, NULL)) {
        return 0;
    }
    return snprintf(path, size, "%s", temporary) < (int)size;
#else
    if (size < sizeof("/tmp/rage-host-disc-XXXXXX")) return 0;
    memcpy(path, "/tmp/rage-host-disc-XXXXXX",
           sizeof("/tmp/rage-host-disc-XXXXXX"));
    return mkdtemp(path) != NULL;
#endif
}

static int ConfigureDiscImage(const char *discPath) {
    char arg[1200];
    char *argv[3];

    if (discPath != NULL) {
        snprintf(arg, sizeof(arg), "disc.image=%s", discPath);
        argv[0] = "host_disc_error_tests";
        argv[1] = "--set";
        argv[2] = arg;
        return RuntimeConfigInit(3, argv);
    }
    argv[0] = "host_disc_error_tests";
    argv[1] = "--set";
    argv[2] = "disc.choose=true";
    return RuntimeConfigInit(3, argv);
}

int main(void) {
    char dir[1024];
    char isoPath[1200];
    char missingPath[1200];
    char badCuePath[1200];
    char missingTrackCuePath[1200];
    char garbageBinPath[1200];
    char ridgeBinPath[1200];
    char truncatedBinPath[1200];
    char validBinPath[1200];
    char badChdPath[1200];
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    CHECK(MakeTemporaryDirectory(dir, sizeof(dir)));
#ifdef _WIN32
    CHECK(_putenv_s("APPDATA", dir) == 0);
#else
    CHECK(setenv("HOME", dir, 1) == 0);
    CHECK(setenv("XDG_CONFIG_HOME", dir, 1) == 0);
#endif
    snprintf(isoPath, sizeof(isoPath), "%s%cdisc.iso", dir, sep);
    snprintf(missingPath, sizeof(missingPath), "%s%cmissing.cue", dir, sep);
    snprintf(badCuePath, sizeof(badCuePath), "%s%caudio-only.cue", dir, sep);
    snprintf(missingTrackCuePath, sizeof(missingTrackCuePath),
             "%s%cmissing-track.cue", dir, sep);
    snprintf(garbageBinPath, sizeof(garbageBinPath), "%s%cgarbage.bin", dir,
             sep);
    snprintf(ridgeBinPath, sizeof(ridgeBinPath), "%s%cridge-racer.bin", dir,
             sep);
    snprintf(truncatedBinPath, sizeof(truncatedBinPath), "%s%ctruncated.bin",
             dir, sep);
    snprintf(validBinPath, sizeof(validBinPath), "%s%crage-racer.bin", dir,
             sep);
    snprintf(badChdPath, sizeof(badChdPath), "%s%ccorrupt.chd", dir, sep);

    CHECK(WriteTextFile(isoPath, "not an iso"));
    CHECK(WriteTextFile(badCuePath,
                        "FILE \"track01.bin\" BINARY\n"
                        "  TRACK 01 AUDIO\n"
                        "    INDEX 01 00:00:00\n"));
    CHECK(WriteTextFile(missingTrackCuePath,
                        "FILE \"nonexistent-track01.bin\" BINARY\n"
                        "  TRACK 01 MODE2/2352\n"
                        "    INDEX 01 00:00:00\n"));
    CHECK(WriteTextFile(garbageBinPath, "not a raw playstation disc"));
    CHECK(WriteDiscBin(ridgeBinPath, 0, 0));
    CHECK(WriteDiscBin(truncatedBinPath, 1, 13));
    CHECK(WriteDiscBin(validBinPath, 1, 0));
    CHECK(WriteTextFile(badChdPath, "not a chd"));

    /* 1. Unsupported extension */
    CHECK(ConfigureDiscImage(isoPath));
    CHECK(!HostInitDisc());
    CHECK(!HostDiscUserCancelled());
    CHECK(strstr(HostDiscLastError(), "Unsupported disc image file extension") !=
          NULL);

    /* 2. Missing file */
    CHECK(ConfigureDiscImage(missingPath));
    CHECK(!HostInitDisc());
    CHECK(!HostDiscUserCancelled());
    CHECK(strstr(HostDiscLastError(),
                 "Cannot find or read the selected disc image") != NULL);

    /* 3. CUE without a MODE1/2352 or MODE2/2352 data track */
    CHECK(ConfigureDiscImage(badCuePath));
    CHECK(!HostInitDisc());
    CHECK(strstr(HostDiscLastError(),
                 "does not declare a raw 2352-byte data track") != NULL);

    /* 4. CUE referencing a missing .bin file */
    CHECK(ConfigureDiscImage(missingTrackCuePath));
    CHECK(!HostInitDisc());
    CHECK(strstr(HostDiscLastError(),
                 "references a missing data track .bin file") != NULL);

    /* 5. Non-ISO .bin file */
    CHECK(ConfigureDiscImage(garbageBinPath));
    CHECK(!HostInitDisc());
    CHECK(strstr(HostDiscLastError(),
                 "does not contain a valid PlayStation ISO-9660 data track") !=
          NULL);

    /* 6. Valid PS1 ISO for another game (e.g. Ridge Racer, missing RAGE.BIN) */
    CHECK(ConfigureDiscImage(ridgeBinPath));
    CHECK(!HostInitDisc());
    CHECK(strstr(HostDiscLastError(),
                 "is not Rage Racer (missing RAGE.BIN / RAGE.STR on disc)") !=
          NULL);

    /* 7. Valid Rage Racer ISO .bin with non-multiple-of-2352 size */
    CHECK(ConfigureDiscImage(truncatedBinPath));
    CHECK(!HostInitDisc());
    CHECK(strstr(HostDiscLastError(),
                 "size is not a multiple of 2352-byte raw CD sectors") != NULL);

    /* 8. Corrupt CHD */
    CHECK(ConfigureDiscImage(badChdPath));
    CHECK(!HostInitDisc());
    CHECK(strstr(HostDiscLastError(), "Cannot open CHD disc image") != NULL);

    /* 9. Interactive picker: wrong disc shows error dialog and retries, then
     *    accepts valid Rage Racer disc on the second attempt. */
    memset(&g_PickerState, 0, sizeof(g_PickerState));
    g_PickerState.paths[0] = ridgeBinPath;
    g_PickerState.results[0] = 1;
    g_PickerState.paths[1] = validBinPath;
    g_PickerState.results[1] = 1;
    g_PickerState.totalSteps = 2;
    CHECK(ConfigureDiscImage(NULL));
    CHECK(HostInitDisc());
    CHECK(!HostDiscUserCancelled());
    CHECK(g_PickerState.step == 2);
    CHECK(g_PickerState.dialogCalls == 1);
    CHECK(strstr(g_PickerState.lastDialogTitle, "Invalid Disc Image") != NULL);
    CHECK(strstr(g_PickerState.lastDialogMessage, "is not Rage Racer") != NULL);

    /* 10. Interactive picker: wrong disc shows error dialog, then user cancels
     *     on the second attempt. */
    memset(&g_PickerState, 0, sizeof(g_PickerState));
    g_PickerState.paths[0] = ridgeBinPath;
    g_PickerState.results[0] = 1;
    g_PickerState.paths[1] = NULL;
    g_PickerState.results[1] = 0;
    g_PickerState.totalSteps = 2;
    CHECK(ConfigureDiscImage(NULL));
    CHECK(!HostInitDisc());
    CHECK(HostDiscUserCancelled());
    CHECK(g_PickerState.step == 2);
    CHECK(g_PickerState.dialogCalls == 1);

    {
        char savedConfig[1200];
        char configDir[1200];
        if (PlatformUserConfigPath("disc-cue-path", savedConfig,
                                   sizeof(savedConfig))) {
            remove(savedConfig);
        }
        if (PlatformUserConfigDirectory(configDir, sizeof(configDir))) {
            rmdir(configDir);
        }
#ifdef __APPLE__
        snprintf(configDir, sizeof(configDir),
                 "%s/Library/Application Support", dir);
        rmdir(configDir);
        snprintf(configDir, sizeof(configDir), "%s/Library", dir);
        rmdir(configDir);
#endif
    }
    remove(isoPath);
    remove(badCuePath);
    remove(missingTrackCuePath);
    remove(garbageBinPath);
    remove(ridgeBinPath);
    remove(truncatedBinPath);
    remove(validBinPath);
    remove(badChdPath);
    CHECK(rmdir(dir) == 0);
    puts("host disc error handling reports actionable diagnostics and retries picker");
    return 0;
}
