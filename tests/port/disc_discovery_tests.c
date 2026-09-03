#include "disc_discovery.h"

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
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

typedef struct ValidationState {
    int attempts;
} ValidationState;

static int AcceptSecondImage(void *context, const char *path) {
    ValidationState *state = context;
    (void)path;
    state->attempts++;
    return state->attempts == 2;
}

static int WriteFixture(const char *directory, const char *name,
                        const char *contents) {
    char path[1024];
    FILE *file;
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif
    int written = snprintf(path, sizeof(path), "%s%c%s", directory,
                           separator, name);
    if (written < 0 || (size_t)written >= sizeof(path))
        return 0;
    file = fopen(path, "wb");
    if (file == NULL) return 0;
    if (fputs(contents, file) < 0) {
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
        GetTempFileNameA(base, "rdd", 0, temporary) == 0 ||
        !DeleteFileA(temporary) || !CreateDirectoryA(temporary, NULL)) {
        return 0;
    }
    {
        int written = snprintf(path, size, "%s", temporary);
        return written >= 0 && (size_t)written < size;
    }
#else
    if (size < sizeof("/tmp/rage-disc-discovery-XXXXXX")) return 0;
    memcpy(path, "/tmp/rage-disc-discovery-XXXXXX",
           sizeof("/tmp/rage-disc-discovery-XXXXXX"));
    return mkdtemp(path) != NULL;
#endif
}

static void RemoveFixture(const char *directory, const char *name) {
    char path[1024];
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif
    if (snprintf(path, sizeof(path), "%s%c%s", directory, separator, name) >= 0)
        remove(path);
}

static int TestDiscoveryContinuesAfterRejection(void) {
    char directory[1024];
    char selected[1024];
    ValidationState state = {0};

    CHECK(MakeTemporaryDirectory(directory, sizeof(directory)));
    CHECK(WriteFixture(directory, "notes.txt", ""));
    CHECK(WriteFixture(directory, "RAGE.BIN", ""));
    CHECK(WriteFixture(directory, "game.cue", ""));
    CHECK(DiscDiscoverImage(directory, selected, sizeof(selected),
                            AcceptSecondImage, &state));
    CHECK(DiscPathIsSupportedImage(selected));
    CHECK(state.attempts == 2);
    RemoveFixture(directory, "notes.txt");
    RemoveFixture(directory, "RAGE.BIN");
    RemoveFixture(directory, "game.cue");
    CHECK(rmdir(directory) == 0);
    return 0;
}

static int TestSavedPathRejectsTruncation(void) {
    char directory[1024];
    char config[1024];
    char path[64];
    int written;
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif

    CHECK(MakeTemporaryDirectory(directory, sizeof(directory)));
    CHECK(WriteFixture(directory, "saved", "a/complete/game.cue\r\n"));
    written = snprintf(config, sizeof(config), "%s%csaved", directory,
                       separator);
    CHECK(written >= 0 && (size_t)written < sizeof(config));
    memset(path, 'x', sizeof(path));
    CHECK(!DiscReadSavedPath(config, path, 8));
    CHECK(path[0] == '\0');
    CHECK(DiscReadSavedPath(config, path, sizeof(path)));
    CHECK(strcmp(path, "a/complete/game.cue") == 0);
    RemoveFixture(directory, "saved");
    strcpy(path, "stale.cue");
    CHECK(!DiscReadSavedPath(config, path, sizeof(path)));
    CHECK(path[0] == '\0');
    strcpy(path, "stale.cue");
    CHECK(!DiscReadSavedPath(NULL, path, sizeof(path)));
    CHECK(path[0] == '\0');
    path[0] = 'x';
    CHECK(!DiscReadSavedPath(config, path, 1));
    CHECK(path[0] == '\0');
    CHECK(rmdir(directory) == 0);
    return 0;
}

static int TestFileIdentity(void) {
    char directory[1024];
    char alias[1024];
    char first[1024];
    char second[1024];
    int written;
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif

    CHECK(MakeTemporaryDirectory(directory, sizeof(directory)));
    CHECK(WriteFixture(directory, "first.bin", "first"));
    CHECK(WriteFixture(directory, "second.bin", "second"));
    written = snprintf(first, sizeof(first), "%s%cfirst.bin", directory,
                       separator);
    CHECK(written >= 0 && (size_t)written < sizeof(first));
    written = snprintf(second, sizeof(second), "%s%csecond.bin", directory,
                       separator);
    CHECK(written >= 0 && (size_t)written < sizeof(second));
    written = snprintf(alias, sizeof(alias), "%s%calias.bin", directory,
                       separator);
    CHECK(written >= 0 && (size_t)written < sizeof(alias));
#ifdef _WIN32
    CHECK(CreateHardLinkA(alias, first, NULL));
#else
    CHECK(link(first, alias) == 0);
#endif
    CHECK(DiscPathsReferToSameFile(first, first));
    CHECK(DiscPathsReferToSameFile(first, alias));
    CHECK(!DiscPathsReferToSameFile(first, second));
    CHECK(!DiscPathsReferToSameFile(first, "missing.bin"));
    RemoveFixture(directory, "alias.bin");
    RemoveFixture(directory, "first.bin");
    RemoveFixture(directory, "second.bin");
    CHECK(rmdir(directory) == 0);
    return 0;
}

static int TestExtensionsAndArguments(void) {
    char path[8] = "dirty";

    CHECK(DiscPathIsCue("game.CUE"));
    CHECK(DiscPathIsChd("game.ChD"));
    CHECK(DiscPathIsBin("game.bin"));
    CHECK(!DiscPathIsSupportedImage("game.iso"));
    CHECK(!DiscPathIsSupportedImage(NULL));
    CHECK(!DiscDiscoverImage(NULL, path, sizeof(path), AcceptSecondImage,
                             NULL));
    CHECK(path[0] == '\0');
    strcpy(path, "dirty");
    CHECK(!DiscDiscoverImage(".", path, sizeof(path), NULL, NULL));
    CHECK(path[0] == '\0');
    return 0;
}

int main(void) {
    CHECK(TestDiscoveryContinuesAfterRejection() == 0);
    CHECK(TestSavedPathRejectsTruncation() == 0);
    CHECK(TestFileIdentity() == 0);
    CHECK(TestExtensionsAndArguments() == 0);
    puts("disc discovery validates candidates instead of trusting extensions");
    return 0;
}
