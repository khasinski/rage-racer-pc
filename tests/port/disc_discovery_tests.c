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

static int Touch(const char *directory, const char *name) {
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
    CHECK(Touch(directory, "notes.txt"));
    CHECK(Touch(directory, "RAGE.BIN"));
    CHECK(Touch(directory, "game.cue"));
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

static int TestExtensionsAndArguments(void) {
    char path[8] = "dirty";

    CHECK(DiscPathIsCue("game.CUE"));
    CHECK(DiscPathIsChd("game.ChD"));
    CHECK(DiscPathIsBin("game.bin"));
    CHECK(!DiscPathIsSupportedImage("game.iso"));
    CHECK(!DiscPathIsSupportedImage(NULL));
    CHECK(!DiscDiscoverImage(NULL, path, sizeof(path), AcceptSecondImage,
                             NULL));
    return 0;
}

int main(void) {
    CHECK(TestDiscoveryContinuesAfterRejection() == 0);
    CHECK(TestExtensionsAndArguments() == 0);
    puts("disc discovery validates candidates instead of trusting extensions");
    return 0;
}
