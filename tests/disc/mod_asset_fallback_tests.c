#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define unlink _unlink
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

#include "archive_index.h"
#include "mod_assets.h"
#include "render/mod_manifest.h"

static size_t s_room;
static int s_patchCalls;
static int failures;
static const char *configuredDirectory;

#define EXPECT(value) do { if (!(value)) { failures++;                         \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__,  \
            #value);                                                           \
} } while (0)

const char *RuntimeConfigGet(const char *key) {
    return strcmp(key, "mods.directory") == 0 ? configuredDirectory : NULL;
}

static int MakeDirectory(const char *path) {
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0700);
#endif
}

static int MakeTemporaryDirectory(char *path, size_t capacity) {
#ifdef _WIN32
    char temporary[MAX_PATH];
    DWORD length = GetTempPathA(sizeof(temporary), temporary);
    if (length == 0 || length >= sizeof(temporary)) return 0;
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        int written = snprintf(path, capacity, "%srage-mod-%lu-%llu-%u",
                               temporary, (unsigned long)GetCurrentProcessId(),
                               (unsigned long long)GetTickCount64(), attempt);
        if (written < 0 || (size_t)written >= capacity) return 0;
        if (CreateDirectoryA(path, NULL)) return 1;
        if (GetLastError() != ERROR_ALREADY_EXISTS) return 0;
    }
    return 0;
#else
    int written = snprintf(path, capacity, "/tmp/rage-mod-fallback-XXXXXX");
    return written > 0 && (size_t)written < capacity && mkdtemp(path) != NULL;
#endif
}

size_t PortAssetRoomAt(const void *at) {
    (void)at;
    return s_room;
}

int TexturePatchAsset(const char *directory, int assetIndex,
                          unsigned char *data, size_t size) {
    (void)directory;
    (void)assetIndex;
    (void)data;
    (void)size;
    s_patchCalls++;
    return 0;
}

static int WriteFixtureFile(const char *path, const unsigned char *data, size_t size) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) return 0;
    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

int main(int argc, char **argv) {
    char root[512];
    char raw[1024], asset0[2048], asset1[2048], manifestPath[1024];
    unsigned char destination[16];
    static const unsigned char valid[] = {1, 2, 3, 4, 5, 6, 7, 8};
    static const unsigned char unaligned[] = {1, 2, 3, 4, 5, 6};
    static const unsigned char oversized[32] = {9};

    if (!MakeTemporaryDirectory(root, sizeof(root))) return EXIT_FAILURE;
    snprintf(raw, sizeof(raw), "%s/raw", root);
    EXPECT(MakeDirectory(raw) == 0);
    snprintf(asset0, sizeof(asset0), "%s/asset_000.bin", raw);
    snprintf(asset1, sizeof(asset1), "%s/asset_001.bin", raw);
    EXPECT(WriteFixtureFile(asset0, valid, sizeof(valid)));
    EXPECT(WriteFixtureFile(asset1, oversized, sizeof(oversized)));
    configuredDirectory = root;
    snprintf(manifestPath, sizeof(manifestPath), "%s/mod.toml", root);
    if (argc == 2) {
        const char *manifest = "[mod]\nschema_version=1\nid=\"shared\"\n"
                               "[textures]\n\"track.big1\"=\"a.png\"";
        int disabled = strcmp(argv[1], "invalid") == 0 ||
                       strcmp(argv[1], "future") == 0 ||
                       strcmp(argv[1], "unreadable") == 0;
        int unreadable = strcmp(argv[1], "unreadable") == 0;
        int semanticOnly = strcmp(argv[1], "semantic") == 0;
        if (strcmp(argv[1], "invalid") == 0)
            manifest = "[textures]\n\"track.big1\"=\"../bad.png\"";
        if (strcmp(argv[1], "future") == 0)
            manifest = "[mod]\nschema_version=2";
        if (unreadable) EXPECT(MakeDirectory(manifestPath) == 0);
        else EXPECT(WriteFixtureFile(manifestPath, (const unsigned char *)manifest, strlen(manifest)));
        if (semanticOnly) EXPECT(unlink(asset0) == 0);
        memset(destination, 0xA5, sizeof(destination));
        s_room = sizeof(destination);
        /* Exercise raw-first and semantic-first entry paths in fresh processes. */
        if (disabled) EXPECT(ModAssetLoad(0, destination, 2) == 0);
        const RageModManifest *shared = ModAssetsManifest();
        EXPECT((shared == NULL) == disabled);
        EXPECT((ModAssetsDirectory() == NULL) == disabled);
        EXPECT(ModAssetsManifest() == shared);
        if (shared != NULL) {
            EXPECT(shared->schemaVersion == 1 && shared->textureCount == 1);
            EXPECT(strcmp(shared->id, "shared") == 0);
        }
        if (disabled || semanticOnly) {
            EXPECT(ModAssetLoad(0, destination, 2) == 0);
            EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);
        } else {
            EXPECT(ModAssetLoad(0, destination, 2) == 8);
            EXPECT(memcmp(destination, valid, sizeof(valid)) == 0);
        }
        ModPatchTextures(0, destination, sizeof(destination));
        EXPECT(s_patchCalls == (!disabled && !semanticOnly ? 1 : 0));
        /* Once selected, edits on disk must not switch the provider contract
         * midway through a session or mutate a borrowed renderer manifest. */
        if (unreadable) EXPECT(rmdir(manifestPath) == 0);
        static const char changed[] = "[mod]\nschema_version=9";
        EXPECT(WriteFixtureFile(manifestPath, (const unsigned char *)changed, sizeof(changed) - 1));
        EXPECT(ModAssetsManifest() == shared);
        if (shared != NULL) EXPECT(shared->schemaVersion == 1);
        /* All consumers drop their borrows before ending the asset session. */
        shared = NULL;
        ModAssetsShutdown();
        ModAssetsShutdown();
        EXPECT(ModAssetsManifest() == NULL); /* Now sees unsupported schema 9. */
        EXPECT(ModAssetsDirectory() == NULL);
        memset(destination, 0xA5, sizeof(destination));
        EXPECT(ModAssetLoad(0, destination, 2) == 0);
        EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);
        static const char repaired[] = "[mod]\nschema_version=1\nid=\"repaired\"";
        EXPECT(WriteFixtureFile(manifestPath, (const unsigned char *)repaired,
                                sizeof(repaired) - 1));
        EXPECT(ModAssetsManifest() == NULL); /* No implicit hot reload. */
        ModAssetsShutdown();
        shared = ModAssetsManifest();
        EXPECT(shared != NULL && strcmp(shared->id, "repaired") == 0);
        EXPECT(shared != NULL && shared->textureCount == 0);
        shared = NULL;
        ModAssetsShutdown();
        configuredDirectory = NULL;
        EXPECT(ModAssetsDirectory() == NULL && ModAssetsManifest() == NULL);
        configuredDirectory = root;
        EXPECT(ModAssetsDirectory() == NULL); /* Config changes need shutdown. */
        ModAssetsShutdown();
        EXPECT(ModAssetsDirectory() != NULL);
        EXPECT(ModAssetsManifest() != NULL);
        unlink(manifestPath);
        goto cleanup;
    }
    EXPECT(ModAssetsManifest() == NULL); /* Existing raw-only packs still work. */

    memset(destination, 0xA5, sizeof(destination));
    s_room = sizeof(destination);
    EXPECT(ModAssetLoad(0, destination, 2) == 8);
    EXPECT(memcmp(destination, valid, sizeof(valid)) == 0);

    memset(destination, 0xA5, sizeof(destination));
    s_room = 8;
    EXPECT(ModAssetLoad(1, destination, 2) == 0);
    EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);

    s_room = 0;
    EXPECT(ModAssetLoad(0, destination, 2) == 0);
    EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);

    EXPECT(WriteFixtureFile(asset0, unaligned, sizeof(unaligned)));
    s_room = sizeof(destination);
    EXPECT(ModAssetLoad(0, destination, 2) == 0);
    EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);

    EXPECT(ModAssetLoad(2, destination, 2) == 0);
    EXPECT(ModAssetLoad(-1, destination, 2) == 0);
    EXPECT(ModAssetLoad(RAGE_ARCHIVE_INDEX_ENTRY_COUNT, destination, 2) == 0);
    ModPatchTextures(-1, destination, sizeof(destination));
    ModPatchTextures(RAGE_ARCHIVE_INDEX_ENTRY_COUNT, destination,
                     sizeof(destination));
    EXPECT(s_patchCalls == 0);
    ModPatchTextures(0, destination, sizeof(destination));
    EXPECT(s_patchCalls == 1);
cleanup:
    ModAssetsShutdown();
    ModAssetsShutdown();
    unlink(asset0);
    unlink(asset1);
    rmdir(raw);
    rmdir(root);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
