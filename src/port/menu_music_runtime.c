#include "menu_music_runtime.h"

#include "archive_index.h"
#include "host_disc.h"
#include "menu_music_asset.h"
#include "menu_music_render.h"
#include "platform_paths.h"
#include "runtime_config.h"

#include <psyz/audio.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

enum { MENU_MUSIC_ASSET_INDEX = 7 };

static int RenderMenuMusic(const char *path) {
    RageArchiveIndexEntry entries[RAGE_ARCHIVE_INDEX_ENTRY_COUNT];
    RageArchiveIndexEntry entry;
    MenuMusicAsset asset;
    unsigned char *data;
    unsigned tickRate;
    int ok;

    if (!HostLoadArchiveIndex(entries, RAGE_ARCHIVE_INDEX_ENTRY_COUNT))
        return 0;
    entry = entries[MENU_MUSIC_ASSET_INDEX];
    if (entry.size == 0) return 0;
    data = malloc(entry.size);
    if (data == NULL) return 0;
    ok = HostLoadAsset(entry.byteOffset, entry.size, data) > 0 &&
         MenuMusicAssetOpen(data, entry.size, &asset) &&
         (tickRate = MenuMusicTickRate(&asset)) != 0 &&
         MenuMusicRenderWav(&asset, tickRate, path);
    free(data);
    return ok;
}

int MenuMusicPrepare(void) {
    const char *configured = RuntimeConfigGet("audio.menu_music");
    char generated[PATH_MAX];
    char directory[PATH_MAX];
    char filename[64];
    const char *path = configured;

    if (path == NULL || path[0] == '\0') {
        int written = snprintf(filename, sizeof(filename),
                               "menu_music.v9.%s.wav", HostDiscRegion());
        if (written < 0 || (size_t)written >= sizeof(filename)) return 0;
        if (!PlatformUserConfigDirectory(directory, sizeof(directory)) ||
            !PlatformEnsureDirectory(directory) ||
            !PlatformUserConfigPath(filename, generated,
                                    sizeof(generated)))
            return 0;
        path = generated;
    }
    if (Psyz_PcmMusicLoad(path) == 0) return 1;
    if (!RenderMenuMusic(path) || Psyz_PcmMusicLoad(path) != 0) {
        remove(path);
        fprintf(stderr, "rage-port: cannot generate menu music PCM: %s\n",
                path);
        return 0;
    }
    fprintf(stderr, "rage-port: generated menu music PCM: %s\n", path);
    return 1;
}
