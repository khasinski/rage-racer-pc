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
    RAGE_MOD_MANIFEST_MAX_REQUIREMENTS = 16,
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
    char requirements[RAGE_MOD_MANIFEST_MAX_REQUIREMENTS][RAGE_MOD_MANIFEST_ID_CAPACITY];
    size_t requirementCount;
    RageModTextureOverride textures[RAGE_MOD_MANIFEST_MAX_TEXTURES];
    size_t textureCount;
    RageModMaterialOverride materials[RAGE_MOD_MANIFEST_MAX_MATERIALS];
    size_t materialCount;
    size_t errorLine;
    RageModManifestError error;
} RageModManifest;

/* Small TOML subset: [mod] id/schema_version/requires, [textures], [materials].
 * requires is a single-line array of unique semantic mod IDs. The parser
 * records requirements; the session loader must satisfy them before use.
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
 * request to silently try a lower-priority manifest entry. All lookup APIs
 * reject failed/unsupported manifests and counts beyond schema capacity;
 * unknown channel bits produce an empty resolution. */
RageModResolution ModManifestResolve(const RageModManifest *manifest,
    const char *exactId, const char *baseId, unsigned channels);

enum { RAGE_MOD_MAX_SELECTED = 16 };
typedef enum RageModOrderError {
    RAGE_MOD_ORDER_OK, RAGE_MOD_ORDER_INVALID, RAGE_MOD_ORDER_DUPLICATE_ID,
    RAGE_MOD_ORDER_MISSING_REQUIREMENT, RAGE_MOD_ORDER_CYCLE
} RageModOrderError;
typedef struct RageModOrder {
    size_t indices[RAGE_MOD_MAX_SELECTED], count;
    RageModOrderError error;
    size_t modIndex, requirementIndex;
} RageModOrder;
/* Stable depth-first dependency order; input order breaks unrelated ties.
 * Output contains input indices, never borrowed pointers. Failure publishes
 * no order, only diagnostics. A sole legacy unnamed manifest is permitted;
 * selections of two or more manifests require unique nonempty IDs. */
int ModManifestBuildOrder(const RageModManifest *const *manifests,
                          size_t count, RageModOrder *out);
const char *ModManifestOrderErrorString(RageModOrderError error);

#endif
