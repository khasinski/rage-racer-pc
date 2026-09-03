#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "mod_assets.h"
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
static int s_legacyLayout;
static int s_initialized;

static void ModAssetsInit(void) {
    char probe[1024];
    FILE *test;
    int written;

    if (s_initialized) return;
    s_initialized = 1;
    s_directory = RuntimeConfigGet("mods.directory");
    if (s_directory != NULL && s_directory[0] == '\0') s_directory = NULL;
    if (s_directory == NULL) return;
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

static FILE *ModOpen(int index, long *size) {
    char path[1024];
    FILE *file;
    int written;

    if (s_directory == NULL || !s_legacyLayout || index < 0 || size == NULL) {
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

    if (destination == NULL || index < 0) return 0;
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
        static int announced[512];
        if (index >= 0 && index < 512 && !announced[index]) {
            announced[index] = 1;
            fprintf(stderr, "rage-port: asset %d overridden, %u -> %ld bytes\n",
                    index, originalSize, size);
        }
    }
    return (int)size;
}

/* Apply the mod directory's edited images to an asset already in memory. */
void ModPatchTextures(int index, void *data, size_t size) {
    ModAssetsInit();
    if (s_directory == NULL || !s_legacyLayout) return;
    TexturePatchAsset(s_directory, index, data, size);
}
