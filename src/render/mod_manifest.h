#ifndef RAGE_MOD_MANIFEST_H
#define RAGE_MOD_MANIFEST_H

#include <stddef.h>

enum {
    RAGE_MOD_MANIFEST_SCHEMA_VERSION = 1,
    RAGE_MOD_MANIFEST_MAX_TEXTURES = 512,
    RAGE_MOD_MANIFEST_MAX_MATERIALS = 512,
    RAGE_MOD_MANIFEST_ID_CAPACITY = 96,
    RAGE_MOD_MANIFEST_KEY_CAPACITY = 160,
    RAGE_MOD_MANIFEST_PATH_CAPACITY = 512,
    RAGE_MOD_MANIFEST_PROPERTIES_CAPACITY = 256,
};

typedef enum RageModManifestError {
    RAGE_MOD_MANIFEST_OK,
    RAGE_MOD_MANIFEST_INVALID,
    RAGE_MOD_MANIFEST_UNSUPPORTED_VERSION,
} RageModManifestError;

typedef struct RageModTextureOverride {
    char key[RAGE_MOD_MANIFEST_KEY_CAPACITY];
    char path[RAGE_MOD_MANIFEST_PATH_CAPACITY];
} RageModTextureOverride;

typedef struct RageModMaterialOverride {
    char key[RAGE_MOD_MANIFEST_KEY_CAPACITY];
    char properties[RAGE_MOD_MANIFEST_PROPERTIES_CAPACITY];
} RageModMaterialOverride;

typedef struct RageModManifest {
    unsigned schemaVersion;
    char id[RAGE_MOD_MANIFEST_ID_CAPACITY];
    RageModTextureOverride textures[RAGE_MOD_MANIFEST_MAX_TEXTURES];
    size_t textureCount;
    RageModMaterialOverride materials[RAGE_MOD_MANIFEST_MAX_MATERIALS];
    size_t materialCount;
    size_t errorLine;
    RageModManifestError error;
} RageModManifest;

/* Small TOML subset: [mod] id/schema_version, [textures], [materials].
 * Missing schema_version means legacy schema 1. Unsupported versions fail;
 * failure clears all content and retains only error/errorLine diagnostics.
 * Output owns its values; lookup pointers are borrowed until parse/reset. */
int ModManifestParse(const char *text, size_t size, RageModManifest *out);
const char *ModManifestErrorString(RageModManifestError error);
const char *ModManifestFindTexture(const RageModManifest *manifest,
                                      const char *semanticId);
const char *ModManifestFindMaterialProperties(
    const RageModManifest *manifest, const char *semanticId);

typedef struct RageModResolution {
    const RageModTextureOverride *texture;
    const RageModMaterialOverride *material;
} RageModResolution;
enum { RAGE_MOD_RESOLVE_TEXTURE = 1, RAGE_MOD_RESOLVE_MATERIAL = 2 };

/* Resolve each channel independently: exact variant wins over base, and the
 * last assignment wins within a key. Returned entries belong to manifest,
 * not the query strings, and remain valid until its owner retires it.
 * Resolution performs no I/O: a selected but unreadable texture is not a
 * request to silently try a lower-priority manifest entry. */
RageModResolution ModManifestResolve(const RageModManifest *manifest,
    const char *exactId, const char *baseId, unsigned channels);

#endif
