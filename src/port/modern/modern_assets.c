#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/native_asset_importer.h"
#include "port/mod_assets.h"
#include "port/platform_paths.h"
#include "port/runtime_config.h"
#include "port/sky_panorama_layout.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "render/asset_id.h"
#include "render/car_paint.h"
#include "render/authored_car_surface.h"
#include "render/mod_manifest.h"
#include "render/rmesh_replace.h"
#include "authored_car_data.h"

/* One entry per bank's first definition. More than one car may share a bank;
 * all matching replacements are assembled before the result is cached. */
static RageRuntimeCachedMesh s_authoredCarMesh[RAGE_AUTHORED_CAR_COUNT];

static int AuthoredCarMatches(const AuthoredCarReplacement *car,
                             const RageRenderMeshInstance *instance) {
    return car->assetKey == instance->assetKey &&
           car->assetSet == instance->assetSet;
}

static const RageRuntimeCachedMesh *ModernAuthoredCar(
    const RageRuntimeCachedMesh *base, const RageRenderMeshInstance *instance,
    int imported) {
    RageRuntimeCachedMesh working, *entry;
    void *owned = NULL;
    size_t first, i;
    if (!base) return NULL;
    if (instance->assetSet != RAGE_RENDER_ASSET_MODEL_BANK &&
        instance->assetSet != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) return base;
    for (first = 0; first < RAGE_AUTHORED_CAR_COUNT; first++)
        if (AuthoredCarMatches(&s_authoredCars[first], instance)) break;
    if (first == RAGE_AUTHORED_CAR_COUNT ||
        RuntimeConfigInt("modern.authored_cars", 1, 0, 1) == 0) return base;
    entry = &s_authoredCarMesh[first];
    if (entry->ownedBytes) return entry;
    working = *base;
    for (i = first; i < RAGE_AUTHORED_CAR_COUNT; i++) {
        const AuthoredCarReplacement *car = &s_authoredCars[i];
        RageRuntimeMesh body, next;
        uint32_t map[RAGE_CAR_SURFACE_SOURCE_STRIDE * RAGE_CAR_SURFACE_COUNT];
        size_t j, size;
        void *bytes;
        if (!AuthoredCarMatches(car, instance)) continue;
        for (j = 0; j < sizeof(map)/sizeof(map[0]); j++) map[j] = UINT32_MAX;
        for (j = 0; j < car->materialCount; j++) {
            const AuthoredCarMaterial *material = &car->materials[j];
            int slot = imported ? NativeAssetImporterMaterialSlot(instance,
                material->page, material->clut) : material->cacheSlot;
            if (slot < 0 || slot >= RAGE_CAR_SURFACE_RUNTIME_STRIDE ||
                material->source >= RAGE_CAR_SURFACE_SOURCE_STRIDE) {
                fprintf(stderr, "rage-port: %s asset %u material %u unavailable\n",
                        car->name, car->assetKey, material->source);
                goto failed;
            }
            map[material->source] = (uint32_t)slot;
            for (unsigned surface = 1; surface < RAGE_CAR_SURFACE_COUNT; surface++)
                map[material->source + surface * RAGE_CAR_SURFACE_SOURCE_STRIDE] =
                    (uint32_t)slot + surface * RAGE_CAR_SURFACE_RUNTIME_STRIDE;
        }
        if (!RuntimeMeshOpen(&body, car->bytes, car->byteCount)) goto failed;
        bytes = RuntimeMeshReplace(&working.mesh, car->submesh, &body,
                                   map, sizeof(map)/sizeof(map[0]), &size);
        if (!bytes) goto failed;
        if (!RuntimeMeshOpen(&next, bytes, size)) {
            free(bytes);
            goto failed;
        }
        free(owned);
        owned = bytes;
        working.mesh = next;
        fprintf(stderr, "rage-port: authored %s %s body installed asset=%u (%u triangles)\n",
                car->name, car->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? "player" : "rival",
                car->assetKey, body.indexCount/3);
    }
    working.ownedBytes = owned;
    *entry = working;
    return entry;
failed:
    free(owned);
    return NULL;
}

enum {
    MODERN_ASSET_CACHE_CAPACITY = 4096,
    MODERN_ASSET_MAX_IMAGE_DIMENSION = 16384,
};

static char s_root[1024];
static void *s_indexBytes;
static size_t s_indexSize;
static void *s_environmentIndexBytes;
static size_t s_environmentIndexSize;
static RageRuntimeCachedMesh s_entries[MODERN_ASSET_CACHE_CAPACITY];
static RageRuntimeMeshCache s_cache;
static int s_initialized;
static int s_ready;
static int s_importerSource;
static char s_modRoot[1024];
static RageModManifest s_modManifest;
static int s_modReady;
/* Material sidecars are read synchronously on the render thread. Copy the
 * selected relative path out before releasing their transient file buffer. */
static char s_materialPath[1024];
static char s_paintPath[1024];

static void ModernAssetsInitModProvider(void) {
    const char *root = ModAssetsDirectory();
    char path[sizeof(s_modRoot) + 32];
    void *bytes;
    size_t size, rootLength;
    if (root == NULL || root[0] == '\0') return;
    rootLength = strlen(root);
    if (rootLength >= sizeof(s_modRoot) ||
        rootLength + sizeof("/mod.toml") > sizeof(path)) {
        fprintf(stderr, "rage-port: semantic mod path is too long\n");
        return;
    }
    memcpy(s_modRoot, root, rootLength + 1);
    snprintf(path, sizeof(path), "%s/mod.toml", s_modRoot);
    bytes = SDL_LoadFile(path, &size);
    if (bytes == NULL) return;
    if (!ModManifestParse(bytes, size, &s_modManifest)) {
        fprintf(stderr, "rage-port: invalid semantic mod manifest %s:%zu\n",
                path, s_modManifest.errorLine);
        SDL_free(bytes);
        return;
    }
    SDL_free(bytes);
    s_modReady = 1;
    fprintf(stderr, "rage-port: semantic asset mod %s from %s\n",
            s_modManifest.id[0] != '\0' ? s_modManifest.id : "(unnamed)",
            s_modRoot);
}

static int ModernAssetReadFile(void *context, const char *path,
                               size_t pathLength, const void **bytes,
                               size_t *size) {
    char fullPath[sizeof(s_root) + 1024];
    size_t rootLength = strlen(s_root);
    void *file;
    (void)context;
    if (rootLength + 1 + pathLength + 1 > sizeof(fullPath)) return 0;
    memcpy(fullPath, s_root, rootLength);
    fullPath[rootLength] = '/';
    memcpy(fullPath + rootLength + 1, path, pathLength);
    fullPath[rootLength + 1 + pathLength] = '\0';
    file = SDL_LoadFile(fullPath, size);
    if (file == NULL) return 0;
    *bytes = file;
    return 1;
}

static void ModernAssetFreeFile(void *context, const void *bytes) {
    (void)context;
    SDL_free((void *)bytes);
}

static int ModernAssetsTryRoot(const char *root) {
    char indexPath[sizeof(s_root) + 32];
    char environmentIndexPath[sizeof(s_root) + 32];
    size_t rootLength;
    if (root == NULL || root[0] == '\0') return 0;
    rootLength = strlen(root);
    if (rootLength == 0 || rootLength >= sizeof(s_root) ||
        rootLength + sizeof("/runtime-index.txt") > sizeof(indexPath)) {
        fprintf(stderr, "rage-port: native asset path is too long\n");
        return 0;
    }
    memcpy(s_root, root, rootLength + 1);
    snprintf(indexPath, sizeof(indexPath), "%s/runtime-index.txt", s_root);
    s_indexBytes = SDL_LoadFile(indexPath, &s_indexSize);
    if (s_indexBytes == NULL) return 0;
    if (RuntimeIndexVersion(s_indexBytes, s_indexSize) !=
        RAGE_RUNTIME_INDEX_VERSION) {
        fprintf(stderr,
                "rage-port: native asset cache %s uses an incompatible "
                "material contract; regenerate it with this build\n", s_root);
        SDL_free(s_indexBytes);
        s_indexBytes = NULL;
        s_indexSize = 0;
        return 0;
    }
    RuntimeMeshCacheInit(&s_cache, s_indexBytes, s_indexSize,
                             ModernAssetReadFile, ModernAssetFreeFile, NULL,
                             s_entries, MODERN_ASSET_CACHE_CAPACITY);
    snprintf(environmentIndexPath, sizeof(environmentIndexPath),
             "%s/environment-index.txt", s_root);
    s_environmentIndexBytes = SDL_LoadFile(
        environmentIndexPath, &s_environmentIndexSize);
    s_ready = 1;
    fprintf(stderr, "rage-port: native asset cache %s\n", s_root);
    return 1;
}

int ModernAssetsInit(void) {
    const char *configured;
    char directory[1024];
    char candidate[1024];
    if (s_initialized) return s_ready;
    s_initialized = 1;
    ModernAssetsInitModProvider();
    configured = RuntimeConfigGetForced("modern.assets");
    /* `disc` asks for the importer by name. Without it the only way to reach
     * that path is for no prebuilt cache to exist anywhere the search looks,
     * which makes a test of the importer a test of the tester's directory. */
    if (configured != NULL && strcmp(configured, "disc") == 0) {
        configured = NULL;
    } else if (configured != NULL && configured[0] != '\0') {
        if (ModernAssetsTryRoot(configured)) return 1;
        fprintf(stderr, "rage-port: native asset cache unavailable: %s\n",
                configured);
        return 0;
    }
    if (RuntimeConfigGetForced("modern.assets") == NULL &&
        PlatformExecutableDirectory(NULL, directory, sizeof(directory))) {
        int written = snprintf(candidate, sizeof(candidate), "%s/native-assets",
                               directory);
        if (written > 0 && (size_t)written < sizeof(candidate) &&
            ModernAssetsTryRoot(candidate)) return 1;
#ifdef __APPLE__
        written = snprintf(candidate, sizeof(candidate),
                           "%s/../../../native-assets", directory);
        if (written > 0 && (size_t)written < sizeof(candidate) &&
            ModernAssetsTryRoot(candidate)) return 1;
#endif
    }
    if (NativeAssetImporterReady()) {
        s_importerSource = 1;
        s_ready = 1;
        fprintf(stderr, "rage-port: native assets generated by C importer\n");
        return 1;
    }
    fprintf(stderr,
            "rage-port: could not generate modern renderer assets from the "
            "selected disc image; select a valid Rage Racer CUE or Track 01 BIN\n");
    return 0;
}

int ModernAssetsInitRoot(const char *root) {
    if (s_initialized) return s_ready;
    s_initialized = 1;
    ModernAssetsInitModProvider();
    if (ModernAssetsTryRoot(root)) return 1;
    fprintf(stderr, "rage-port: native asset cache unavailable: %s\n",
            root != NULL ? root : "(null)");
    return 0;
}

void ModernAssetsShutdown(void) {
    size_t i;
    for (i=0;i<sizeof(s_authoredCarMesh)/sizeof(s_authoredCarMesh[0]);i++)
        free((void *)s_authoredCarMesh[i].ownedBytes);
    memset(&s_authoredCarMesh,0,sizeof(s_authoredCarMesh));
    RuntimeMeshCacheRelease(&s_cache);
    NativeAssetImporterShutdown();
    if (s_indexBytes != NULL) SDL_free(s_indexBytes);
    if (s_environmentIndexBytes != NULL) SDL_free(s_environmentIndexBytes);
    s_indexBytes = NULL;
    s_indexSize = 0;
    s_environmentIndexBytes = NULL;
    s_environmentIndexSize = 0;
    s_ready = 0;
    s_importerSource = 0;
    s_initialized = 0;
    s_root[0] = '\0';
    s_modRoot[0] = '\0';
    memset(&s_modManifest, 0, sizeof(s_modManifest));
    s_modReady = 0;
}

const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance) {
    if (!s_ready || instance == NULL) return NULL;
    if (s_importerSource) return ModernAuthoredCar(NativeAssetImporterFind(instance),instance,1);
    return ModernAuthoredCar(RuntimeMeshCacheFind(&s_cache, instance->assetKey,
                                    instance->assetSet),instance,0);
}

int ModernAssetsReady(void) {
    return s_ready;
}

int ModernAssetsLoadSkyImage(uint32_t assetKey, ModernAssetImage *image) {
    const char *bytes = (const char *)s_environmentIndexBytes;
    size_t lineStart = 0, cursor;
    char line[1200];
    unsigned key, width, height;
    char path[1024];
    const void *pixels;
    size_t size, expectedSize;
    if (image == NULL) return 0;
    memset(image, 0, sizeof(*image));
    if (s_importerSource)
        return NativeAssetImporterLoadSky(assetKey, image);
    if (!s_ready || bytes == NULL) return 0;
    for (cursor = 0; cursor <= s_environmentIndexSize; cursor++) {
        if (cursor != s_environmentIndexSize && bytes[cursor] != '\n')
            continue;
        if (cursor > lineStart && bytes[lineStart] != '#') {
            size_t length = cursor - lineStart;
            if (length < sizeof(line)) {
                memcpy(line, bytes + lineStart, length);
                line[length] = '\0';
                if (sscanf(line, "%u %u %u %1023s", &key, &width, &height,
                           path) == 4 && key == assetKey && width != 0 &&
                    height != 0 &&
                    width <= MODERN_ASSET_MAX_IMAGE_DIMENSION &&
                    height <= MODERN_ASSET_MAX_IMAGE_DIMENSION) {
                    expectedSize = (size_t)width * (size_t)height * 4u;
                    if (ModernAssetReadFile(NULL, path, strlen(path), &pixels,
                                            &size)) {
                        if (size == expectedSize) {
                            if (width == 512 && height == 128) {
                                size_t expandedSize = 512u * 256u * 4u;
                                uint8_t *expanded = SDL_malloc(expandedSize);
                                if (expanded == NULL ||
                                    !RageSkyExpandPanorama(
                                        expanded, expandedSize, pixels, size,
                                        g_SkyTileMap, g_SkyRowBase)) {
                                    SDL_free(expanded);
                                    ModernAssetFreeFile(NULL, pixels);
                                    return 0;
                                }
                                ModernAssetFreeFile(NULL, pixels);
                                image->pixels = expanded;
                                image->size = expandedSize;
                                image->width = 512;
                                image->height = 256;
                            } else {
                                image->pixels = (void *)pixels;
                                image->size = size;
                                image->width = width;
                                image->height = height;
                            }
                            return 1;
                        }
                        ModernAssetFreeFile(NULL, pixels);
                    }
                }
            }
        }
        lineStart = cursor + 1;
    }
    return 0;
}

uint32_t ModernAssetsCachedMeshCount(void) {
    if (!s_ready) return 0;
    return s_importerSource ? NativeAssetImporterMeshCount()
                            : s_cache.count;
}

const RageRuntimeMesh *ModernAssetsMeshLookup(
    void *context, const RageRenderMeshInstance *instance) {
    const RageRuntimeCachedMesh *cached;
    (void)context;
    cached = ModernAssetsFind(instance);
    return cached != NULL ? &cached->mesh : NULL;
}

static const char *ModernAssetsFindModMaterialProperties(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant) {
    char exactId[160], baseId[160];
    const char *properties = NULL;

    if (!s_modReady) return NULL;
    if (AssetMaterialVariantId(
            exactId, sizeof(exactId), instance->assetKey,
            instance->assetSet, material, variant)) {
        properties = ModManifestFindMaterialProperties(&s_modManifest,
                                                       exactId);
    }
    if (properties == NULL &&
        AssetMaterialId(baseId, sizeof(baseId), instance->assetKey,
                        instance->assetSet, material)) {
        properties = ModManifestFindMaterialProperties(&s_modManifest,
                                                       baseId);
    }
    return properties;
}

static int ModernAssetsFindMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition) {
    const RageRuntimeCachedMesh *cached;
    const void *mapBytes;
    size_t mapSize;
    int parsed;
    if (definition == NULL || instance == NULL) return 0;
    cached = ModernAssetsFind(instance);
    if (cached == NULL || cached->location.materialPathLength == 1 ||
        cached->location.materialPath[0] == '-' ||
        !ModernAssetReadFile(NULL, cached->location.materialPath,
                             cached->location.materialPathLength,
                             &mapBytes, &mapSize)) return 0;
    parsed = RenderMaterialParse(
        mapBytes, mapSize, material, variant, definition);
    if (parsed && definition->baseColorTexture.length != 0 &&
        definition->baseColorTexture.length < sizeof(s_materialPath) &&
        definition->paintMask.length < sizeof(s_paintPath)) {
        memcpy(s_materialPath, definition->baseColorTexture.text,
               definition->baseColorTexture.length);
        s_materialPath[definition->baseColorTexture.length] = '\0';
        if (definition->paintMask.length != 0) {
            memcpy(s_paintPath, definition->paintMask.text,
                   definition->paintMask.length);
            s_paintPath[definition->paintMask.length] = '\0';
        }
    } else {
        parsed = 0;
    }
    ModernAssetFreeFile(NULL, mapBytes);
    if (!parsed) return 0;
    definition->baseColorTexture.text = s_materialPath;
    definition->paintMask.text = definition->paintMask.length != 0
        ? s_paintPath : NULL;
    if (RuntimeConfigEnabled("diagnostics.modern_asset_trace")) {
        fprintf(stderr,
                "rage-port: native material asset=%u set=%u material=%u "
                "variant=%u path=%s\n",
                instance->assetKey, (unsigned)instance->assetSet, material,
                variant, s_materialPath);
    }
    if (s_modReady) {
        const char *properties = ModernAssetsFindModMaterialProperties(
            instance, material, variant);
        if (properties != NULL && !RenderMaterialParseProperties(
                properties, strlen(properties), definition)) return 0;
    }
    return 1;
}

static int ModernAssetsApplyModMaterialProperties(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition) {
    const char *properties = ModernAssetsFindModMaterialProperties(
        instance, material, variant);

    return properties == NULL || RenderMaterialParseProperties(
        properties, strlen(properties), definition);
}

static int ModernAssetsLoadModImage(const RageRenderMeshInstance *instance,
                                    uint32_t material, uint8_t variant,
                                    ModernAssetImage *image) {
    char exactId[160], baseId[160], fullPath[sizeof(s_modRoot) + 512];
    const char *relativePath;
    SDL_Surface *source = NULL, *converted = NULL;
    uint8_t *pixels = NULL;
    size_t rowSize, size;
    int row;
    if (!s_modReady ||
        !AssetMaterialVariantId(exactId, sizeof(exactId),
                                    instance->assetKey, instance->assetSet,
                                    material, variant) ||
        !AssetMaterialId(baseId, sizeof(baseId), instance->assetKey,
                             instance->assetSet, material)) return 0;
    relativePath = ModManifestFindTexture(&s_modManifest, exactId);
    if (relativePath == NULL)
        relativePath = ModManifestFindTexture(&s_modManifest, baseId);
    if (relativePath == NULL ||
        snprintf(fullPath, sizeof(fullPath), "%s/%s", s_modRoot,
                 relativePath) >= (int)sizeof(fullPath)) return 0;
    source = SDL_LoadPNG(fullPath);
    if (source == NULL) {
        fprintf(stderr, "rage-port: cannot load texture override %s: %s\n",
                fullPath, SDL_GetError());
        return 0;
    }
    converted = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(source);
    if (converted == NULL || converted->w <= 0 || converted->h <= 0 ||
        converted->w > MODERN_ASSET_MAX_IMAGE_DIMENSION ||
        converted->h > MODERN_ASSET_MAX_IMAGE_DIMENSION) goto fail;
    rowSize = (size_t)converted->w * 4u;
    if ((size_t)converted->h > SIZE_MAX / rowSize) goto fail;
    size = rowSize * (size_t)converted->h;
    if (size > UINT32_MAX) goto fail;
    pixels = SDL_malloc(size);
    if (pixels == NULL) goto fail;
    for (row = 0; row < converted->h; row++)
        memcpy(pixels + (size_t)row * rowSize,
               (const uint8_t *)converted->pixels +
                   (size_t)row * (size_t)converted->pitch,
               rowSize);
    image->pixels = pixels;
    image->size = size;
    image->width = (uint32_t)converted->w;
    image->height = (uint32_t)converted->h;
    SDL_DestroySurface(converted);
    fprintf(stderr, "rage-port: native texture override %s <- %s (%ux%u)\n",
            ModManifestFindTexture(&s_modManifest, exactId) != NULL
                ? exactId : baseId,
            relativePath, image->width, image->height);
    return 1;
fail:
    if (pixels != NULL) SDL_free(pixels);
    if (converted != NULL) SDL_DestroySurface(converted);
    fprintf(stderr, "rage-port: invalid texture override %s\n", fullPath);
    return 0;
}

static int ModernAssetsLoadBaseMaterial(const RageRenderMeshInstance *instance,
                             uint32_t material, uint8_t variant,
                             RageRenderMaterial *definition,
                             ModernAssetImage *image) {
    const char *path, *paintPath;
    const void *pixels = NULL;
    size_t pathLength, paintPathLength;
    if (image == NULL || definition == NULL || instance == NULL) return 0;
    memset(image, 0, sizeof(*image));
    if (s_importerSource) {
        ModernAssetImage overrideImage;
        if (!NativeAssetImporterLoadMaterial(
                instance, material, variant, definition, image)) return 0;
        memset(&overrideImage, 0, sizeof(overrideImage));
        if (ModernAssetsLoadModImage(instance, material, variant,
                                     &overrideImage)) {
            ModernAssetsFreeMaterialImage(image);
            *image = overrideImage;
        }
        if (!ModernAssetsApplyModMaterialProperties(
                instance, material, variant, definition)) {
            ModernAssetsFreeMaterialImage(image);
            return 0;
        }
        return 1;
    }
    if (!ModernAssetsFindMaterial(instance, material, variant, definition))
        return 0;
    path = definition->baseColorTexture.text;
    pathLength = definition->baseColorTexture.length;
    paintPath = definition->paintMask.text;
    paintPathLength = definition->paintMask.length;
    if (ModernAssetsLoadModImage(instance, material, variant, image)) return 1;
    if (
        !ModernAssetReadFile(NULL, path, pathLength, &pixels, &image->size) ||
        image->size != 256u * 256u * 4u) {
        if (pixels != NULL) ModernAssetFreeFile(NULL, pixels);
        memset(image, 0, sizeof(*image));
        return 0;
    }
    image->pixels = (void *)pixels;
    image->width = 256;
    image->height = 256;
    if (instance->hasCarPaint && paintPath != NULL) {
        const void *mask = NULL;
        size_t maskSize = 0;
        if (!ModernAssetReadFile(NULL, paintPath, paintPathLength, &mask,
                                 &maskSize) || maskSize != 256u * 256u ||
            !CarPaintApply(image->pixels, mask, 256u * 256u,
                               instance->carPaintColor1,
                               instance->carPaintColor2)) {
            if (mask != NULL) ModernAssetFreeFile(NULL, mask);
            ModernAssetFreeFile(NULL, image->pixels);
            memset(image, 0, sizeof(*image));
            fprintf(stderr,
                    "rage-port: native car paint mask unavailable for %s\n",
                    path);
            return 0;
        }
        ModernAssetFreeFile(NULL, mask);
        if (RuntimeConfigEnabled("diagnostics.modern_asset_trace"))
            fprintf(stderr,
                    "rage-port: native car paint asset=%u material=%u "
                    "colors=%u,%u mask=%s\n",
                    instance->assetKey, material,
                    (unsigned)instance->carPaintColor1,
                    (unsigned)instance->carPaintColor2, paintPath);
    }
    return 1;
}

int ModernAssetsLoadMaterial(const RageRenderMeshInstance *instance,
                             uint32_t material, uint8_t variant,
                             RageRenderMaterial *definition,
                             ModernAssetImage *image) {
    unsigned surface = 0;
    if (instance && (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
                     instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)) {
        surface = material / RAGE_CAR_SURFACE_RUNTIME_STRIDE;
        if (surface >= RAGE_CAR_SURFACE_COUNT) return 0;
        material %= RAGE_CAR_SURFACE_RUNTIME_STRIDE;
    }
    if (!ModernAssetsLoadBaseMaterial(instance, material, variant, definition, image))
        return 0;
    if (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK &&
        (surface == RAGE_CAR_SURFACE_GLASS || surface == RAGE_CAR_SURFACE_DECAL) &&
        !NativeAssetImporterApplyPlayerMarkings(
            surface == RAGE_CAR_SURFACE_GLASS ? 0x3bef : 0x7801, image)) {
        ModernAssetsFreeMaterialImage(image);
        return 0;
    }
    AuthoredCarSurfaceApply(surface, definition);
    AuthoredCarSurfaceTexture(surface, image->pixels, image->size);
    return 1;
}

void ModernAssetsFreeMaterialImage(ModernAssetImage *image) {
    if (image != NULL && image->pixels != NULL)
        ModernAssetFreeFile(NULL, image->pixels);
    if (image != NULL) memset(image, 0, sizeof(*image));
}

void ModernAssetsWarmWorld(const RageRenderWorld *world) {
    uint32_t i;
    if (!s_ready || world == NULL) return;
    for (i = 0; i < world->instanceCount; i++) {
        (void)ModernAssetsFind(&world->instances[i]);
    }
}
