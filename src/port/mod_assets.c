#include <stdio.h>
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

    if (s_initialized) return;
    s_initialized = 1;
    s_directory = RuntimeConfigGet("mods.directory");
    if (s_directory != NULL && s_directory[0] == '\0') s_directory = NULL;
    if (s_directory == NULL) return;
    /* Say plainly when the directory is not the shape rage-extract writes,
     * rather than silently playing the disc and leaving a modder to wonder
     * why nothing changed. */
    snprintf(probe, sizeof(probe), "%s/raw/asset_000.bin", s_directory);
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
    if (s_directory == NULL || !s_legacyLayout) return NULL;
    snprintf(path, sizeof(path), "%s/raw/asset_%03d.bin", s_directory, index);
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
    size_t room;
    size_t loaded;

    ModAssetsInit();
    file = ModOpen(index, &size);
    if (file == NULL) return 0;

    room = PortAssetRoomAt(destination);
    if (room != 0 && (size_t)size > room) {
        fclose(file);
        fprintf(stderr,
                "rage-port: override for asset %d is %ld bytes with %zu available; using the original\n",
                index, size, room);
        return 0;
    }

    loaded = fread(destination, 1, (size_t)size, file);
    fclose(file);
    if (loaded != (size_t)size) {
        fprintf(stderr, "rage-port: override for asset %d could not be read\n", index);
        return 0;
    }
    if ((unsigned int)size != originalSize) {
        static int announced[512];
        if (index >= 0 && index < 512 && !announced[index]) {
            announced[index] = 1;
            fprintf(stderr, "rage-port: asset %d overridden, %u -> %ld bytes\n",
                    index, originalSize, size);
        }
    }
    /* Loads report their size with the low two bits cleared, the way the
     * archive reader does, because callers add it straight onto word-aligned
     * cursors. */
    return (int)((unsigned long)size & ~3ul);
}

/* Apply the mod directory's edited images to an asset already in memory. */
void ModPatchTextures(int index, void *data, size_t size) {
    ModAssetsInit();
    if (s_directory == NULL || !s_legacyLayout) return;
    TexturePatchAsset(s_directory, index, data, size);
}
