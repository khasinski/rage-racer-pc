#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "archive_index.h"
#include "mod_assets.h"
#include "render/mod_manifest.h"
#include "texture_patch.h"
#include "runtime_config.h"

/*
 * Asset overrides read from a directory, laid out the way rage-extract writes
 * one: <directory>/raw/asset_NNN.bin, numbered by archive index.
 *
 * An override is free to be larger than the entry it replaces. Nothing in the
 * loader assumes otherwise: callers advance their cursors by the size a load
 * returns, and the port gives the asset region 64 MB where the console had two,
 * so a pack has room to grow into. What the loader cannot survive is a load
 * running off the end of the buffer it was handed, so that is checked here and
 * refused rather than discovered later as corruption somewhere unrelated.
 */

static const char *s_directory;
static char s_directoryStorage[1024];
static RageModManifest s_manifest;
static int s_manifestReady;
static int s_legacyLayout;
static int s_initialized;
static int s_announced[RAGE_ARCHIVE_INDEX_ENTRY_COUNT];

void ModAssetsShutdown(void) {
    s_directory = NULL;
    memset(s_directoryStorage, 0, sizeof(s_directoryStorage));
    memset(&s_manifest, 0, sizeof(s_manifest));
    memset(s_announced, 0, sizeof(s_announced));
    s_manifestReady = 0;
    s_legacyLayout = 0;
    s_initialized = 0;
}

/* Missing manifests retain legacy-only compatibility. Any other read/parse
 * failure disables the entire directory, before either provider can use it. */
static int ModAssetsReadManifest(void) {
    char path[1100];
    snprintf(path, sizeof(path), "%s/mod.toml", s_directory);
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        if (errno == ENOENT) return 1;
        fprintf(stderr, "rage-port: cannot open mod manifest %s; mod disabled\n", path);
        return 0;
    }
    long size = -1;
    if (fseek(file, 0, SEEK_END) == 0) size = ftell(file);
    /* More than the maximum meaningful content in the bounded schema. */
    if (size < 0 || size > 2 * 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "rage-port: invalid mod manifest size %s; mod disabled\n", path);
        return 0;
    }
    char *bytes = malloc((size_t)size + 1);
    if (bytes == NULL) {
        fclose(file);
        fprintf(stderr, "rage-port: cannot allocate mod manifest %s; mod disabled\n", path);
        return 0;
    }
    size_t readSize = fread(bytes, 1, (size_t)size, file);
    int readFailed = ferror(file);
    int closeFailed = fclose(file) != 0;
    if (readSize != (size_t)size || readFailed || closeFailed) {
        free(bytes);
        fprintf(stderr, "rage-port: cannot read mod manifest %s; mod disabled\n", path);
        return 0;
    }
    int valid = ModManifestParse(bytes, (size_t)size, &s_manifest);
    free(bytes);
    if (!valid) {
        fprintf(stderr, "rage-port: mod manifest %s:%zu: %s; mod disabled\n",
                path, s_manifest.errorLine, ModManifestErrorString(s_manifest.error));
        return 0;
    }
    const RageModManifest *selected[] = {&s_manifest};
    RageModOrder order;
    if (!ModManifestBuildOrder(selected, 1, &order)) {
        fprintf(stderr, "rage-port: mod %s: %s (%s); mod disabled\n",
                s_manifest.id[0] ? s_manifest.id : "(unnamed)",
                ModManifestOrderErrorString(order.error),
                order.requirementIndex < s_manifest.requirementCount
                    ? s_manifest.requirements[order.requirementIndex] : "selection");
        return 0;
    }
    s_manifestReady = 1;
    return 1;
}

static void ModAssetsInit(void) {
    char probe[1024];
    FILE *test;
    int written;

    if (s_initialized) return;
    s_initialized = 1;
    s_directory = RuntimeConfigGet("mods.directory");
    if (s_directory != NULL && s_directory[0] == '\0') s_directory = NULL;
    if (s_directory == NULL) return;
    if (strlen(s_directory) >= sizeof(s_directoryStorage)) {
        fprintf(stderr, "rage-port: mods.directory path is too long\n");
        s_directory = NULL;
        return;
    }
    strcpy(s_directoryStorage, s_directory);
    s_directory = s_directoryStorage;
    if (!ModAssetsReadManifest()) {
        s_directory = NULL;
        return;
    }
    /* Say plainly when the directory is not the shape rage-extract writes,
     * rather than silently playing the disc and leaving a modder to wonder
     * why nothing changed. */
    written = snprintf(probe, sizeof(probe), "%s/raw/asset_000.bin",
                       s_directory);
    if (written < 0 || (size_t)written >= sizeof(probe)) {
        fprintf(stderr, "rage-port: mods.directory path is too long\n");
        s_directory = NULL;
        return;
    }
    test = fopen(probe, "rb");
    if (test == NULL) {
        fprintf(stderr,
                "rage-port: mods.directory %s has no raw/asset_000.bin; legacy archive overrides are disabled\n",
                s_directory);
        return;
    }
    fclose(test);
    s_legacyLayout = 1;
    fprintf(stderr, "rage-port: asset overrides from %s\n", s_directory);
}

const char *ModAssetsDirectory(void) {
    ModAssetsInit();
    return s_directory;
}

const RageModManifest *ModAssetsManifest(void) {
    ModAssetsInit();
    return s_directory != NULL && s_manifestReady ? &s_manifest : NULL;
}

static FILE *ModOpen(int index, long *size) {
    char path[1024];
    FILE *file;
    int written;

    if (s_directory == NULL || !s_legacyLayout ||
        (unsigned)index >= RAGE_ARCHIVE_INDEX_ENTRY_COUNT || size == NULL) {
        return NULL;
    }
    written = snprintf(path, sizeof(path), "%s/raw/asset_%03d.bin",
                       s_directory, index);
    if (written < 0 || (size_t)written >= sizeof(path)) return NULL;
    file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    *size = ftell(file);
    if (*size <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

int ModAssetLoad(int index, void *destination, unsigned int originalSize) {
    long size;
    FILE *file;
    unsigned char *replacement;
    size_t room;
    size_t loaded;
    int closeFailed;

    if (destination == NULL ||
        (unsigned)index >= RAGE_ARCHIVE_INDEX_ENTRY_COUNT) return 0;
    ModAssetsInit();
    file = ModOpen(index, &size);
    if (file == NULL) return 0;

    room = PortAssetRoomAt(destination);
    if (room == 0 || (size_t)size > room || size > INT_MAX || (size & 3) != 0) {
        fclose(file);
        fprintf(stderr,
                "rage-port: override for asset %d has invalid size %ld with %zu available; using the original\n",
                index, size, room);
        return 0;
    }

    replacement = malloc((size_t)size);
    if (replacement == NULL) {
        fclose(file);
        return 0;
    }
    loaded = fread(replacement, 1, (size_t)size, file);
    closeFailed = fclose(file) != 0;
    if (loaded != (size_t)size || closeFailed) {
        free(replacement);
        fprintf(stderr,
                "rage-port: override for asset %d could not be read\n",
                index);
        return 0;
    }
    memcpy(destination, replacement, (size_t)size);
    free(replacement);
    if ((unsigned int)size != originalSize) {
        if (!s_announced[index]) {
            s_announced[index] = 1;
            fprintf(stderr, "rage-port: asset %d overridden, %u -> %ld bytes\n",
                    index, originalSize, size);
        }
    }
    return (int)size;
}

/* Apply the mod directory's edited images to an asset already in memory. */
void ModPatchTextures(int index, void *data, size_t size) {
    if ((unsigned)index >= RAGE_ARCHIVE_INDEX_ENTRY_COUNT) return;
    ModAssetsInit();
    if (s_directory == NULL || !s_legacyLayout) return;
    TexturePatchAsset(s_directory, index, data, size);
}
