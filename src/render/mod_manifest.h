#ifndef RAGE_MOD_MANIFEST_H
#define RAGE_MOD_MANIFEST_H

#include <stddef.h>

enum {
    RAGE_MOD_MANIFEST_MAX_TEXTURES = 512,
    RAGE_MOD_MANIFEST_ID_CAPACITY = 96,
    RAGE_MOD_MANIFEST_KEY_CAPACITY = 160,
    RAGE_MOD_MANIFEST_PATH_CAPACITY = 512,
};

typedef struct RageModTextureOverride {
    char key[RAGE_MOD_MANIFEST_KEY_CAPACITY];
    char path[RAGE_MOD_MANIFEST_PATH_CAPACITY];
} RageModTextureOverride;

typedef struct RageModManifest {
    char id[RAGE_MOD_MANIFEST_ID_CAPACITY];
    RageModTextureOverride textures[RAGE_MOD_MANIFEST_MAX_TEXTURES];
    size_t textureCount;
    size_t errorLine;
} RageModManifest;

/* Parse the deliberately small TOML surface used by mods: [mod] id and a
 * [textures] table of quoted semantic ids to quoted relative PNG paths. */
int RageModManifestParse(const char *text, size_t size, RageModManifest *out);
const char *RageModManifestFindTexture(const RageModManifest *manifest,
                                      const char *semanticId);

#endif
