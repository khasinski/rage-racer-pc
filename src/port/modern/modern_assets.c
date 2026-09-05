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
#include "render/asset_path.h"
#include "render/car_paint.h"
#include "render/mod_manifest.h"

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
static const RageModManifest *s_modManifest;
static int s_modReady;
/* Material sidecars are read synchronously on the render thread. Copy the
 * selected relative path out before releasing their transient file buffer. */
static char s_materialPath[1024];
static char s_paintPath[1024];

static void ModernAssetsInitModProvider(void) {
    const char *root = ModAssetsDirectory();
    size_t rootLength;
    if (root == NULL || root[0] == '\0') return;
    rootLength = strlen(root);
    if (rootLength >= sizeof(s_modRoot)) {
        fprintf(stderr, "rage-port: semantic mod path is too long\n");
        return;
    }
    memcpy(s_modRoot, root, rootLength + 1);
    s_modManifest = ModAssetsManifest();
    if (s_modManifest == NULL) return;
    s_modReady = 1;
    fprintf(stderr, "rage-port: semantic asset mod %s from %s\n",
            s_modManifest->id[0] != '\0' ? s_modManifest->id : "(unnamed)",
            s_modRoot);
}

static int ModernAssetReadFile(void *context, const char *path,
                               size_t pathLength, const void **bytes,
                               size_t *size) {
    char fullPath[sizeof(s_root) + 1024];
    size_t rootLength = strlen(s_root);
    void *file;
    (void)context;
    if (bytes == NULL || size == NULL) return 0;
    *bytes = NULL;
    *size = 0;
    if (!AssetPathIsRelativeFile(path, pathLength)) return 0;
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

static SDL_EnumerationResult SDLCALL ModernFindEnvironmentIndex(
    void *context, const char *directory, const char *name) {
    (void)directory;
    /* Case-insensitive comparison also covers Windows' default filesystem.
     * An oddly cased entry on a case-sensitive filesystem is rejected if the
     * canonical name cannot be opened, rather than treated as absent. */
    if (SDL_strcasecmp(name, "environment-index.txt") == 0) {
        *(int *)context = 1;
        return SDL_ENUM_SUCCESS;
    }
    return SDL_ENUM_CONTINUE;
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
    size_t errorLine;
    if (!RuntimeIndexValidate(s_indexBytes, s_indexSize, &errorLine)) {
        fprintf(stderr, "rage-port: invalid native asset index %s:%zu\n",
                indexPath, errorLine);
        SDL_free(s_indexBytes);
        s_indexBytes = NULL;
        s_indexSize = 0;
        return 0;
    }
    snprintf(environmentIndexPath, sizeof(environmentIndexPath),
             "%s/environment-index.txt", s_root);
    /* SDL_LoadFile/GetPathInfo do not distinguish absence from other errors.
     * Only a successful directory enumeration proves this optional file is
     * absent. Any existing but unreadable entry invalidates the asset root. */
    int environmentExists = 0;
    if (!SDL_EnumerateDirectory(root, ModernFindEnvironmentIndex, &environmentExists)) {
        fprintf(stderr, "rage-port: cannot inspect native asset directory %s: %s\n",
                root, SDL_GetError());
        goto invalidEnvironment;
    }
    if (environmentExists) {
        SDL_PathInfo info;
        if (!SDL_GetPathInfo(environmentIndexPath, &info) || info.type != SDL_PATHTYPE_FILE) {
            fprintf(stderr, "rage-port: environment asset index is not a readable file: %s\n",
                    environmentIndexPath);
            goto invalidEnvironment;
        }
        s_environmentIndexBytes = SDL_LoadFile(environmentIndexPath, &s_environmentIndexSize);
        if (s_environmentIndexBytes == NULL) {
            fprintf(stderr, "rage-port: cannot read environment asset index %s: %s\n",
                    environmentIndexPath, SDL_GetError());
            goto invalidEnvironment;
        }
    }
    if (s_environmentIndexBytes != NULL &&
        !EnvironmentIndexValidate(s_environmentIndexBytes, s_environmentIndexSize,
                                  MODERN_ASSET_MAX_IMAGE_DIMENSION, &errorLine)) {
        fprintf(stderr, "rage-port: invalid environment asset index %s:%zu\n",
                environmentIndexPath, errorLine);
        goto invalidEnvironment;
    }
    RuntimeMeshCacheInit(&s_cache, s_indexBytes, s_indexSize,
                             ModernAssetReadFile, ModernAssetFreeFile, NULL,
                             s_entries, MODERN_ASSET_CACHE_CAPACITY);
    s_ready = 1;
    fprintf(stderr, "rage-port: native asset cache %s\n", s_root);
    return 1;
invalidEnvironment:
    SDL_free(s_environmentIndexBytes);
    s_environmentIndexBytes = NULL;
    s_environmentIndexSize = 0;
    SDL_free(s_indexBytes);
    s_indexBytes = NULL;
    s_indexSize = 0;
    return 0;
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
            "rage-port: modern renderer requires a native asset cache; "
            "place it beside the executable as native-assets or set "
            "modern.assets\n");
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
    s_modManifest = NULL;
    s_modReady = 0;
    /* Full asset-session shutdown only. Presentation/device restart must keep
     * the manifest alive together with the meshes that were derived from it. */
    ModAssetsShutdown();
}

const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance) {
    if (!s_ready || instance == NULL) return NULL;
    if (s_importerSource) return NativeAssetImporterFind(instance);
    return RuntimeMeshCacheFind(&s_cache, instance->assetKey,
                                    instance->assetSet);
}

int ModernAssetsReady(void) {
    return s_ready;
}

int ModernAssetsLoadSkyImage(uint32_t assetKey,
    const RageSkyPanoramaLayout *capturedLayout, ModernAssetImage *image) {
    RageSkyPanoramaLayout layout;
    const char *bytes = (const char *)s_environmentIndexBytes;
    RageRuntimeEnvironmentLocation location;
    const void *pixels;
    size_t size, expectedSize;
    if (image == NULL) return 0;
    memset(image, 0, sizeof(*image));
    if (capturedLayout != NULL) layout = *capturedLayout;
    else RageSkyCapturePanoramaLayout(&layout, g_SkyTileMap, g_SkyRowBase);
    if (s_importerSource)
        return NativeAssetImporterLoadSky(assetKey, &layout, image);
    if (!s_ready || bytes == NULL) return 0;
    if (!EnvironmentIndexFind(bytes, s_environmentIndexSize, assetKey,
                               MODERN_ASSET_MAX_IMAGE_DIMENSION, &location)) return 0;
    expectedSize = (size_t)location.width * location.height * 4u;
    if (!ModernAssetReadFile(NULL, location.path, location.pathLength, &pixels, &size))
        return 0;
    if (size != expectedSize) {
        ModernAssetFreeFile(NULL, pixels);
        return 0;
    }
    if (location.width == 512 && location.height == 128) {
        size_t expandedSize = 512u * 256u * 4u;
        uint8_t *expanded = SDL_malloc(expandedSize);
        if (expanded == NULL || !RageSkyExpandPanoramaLayout(
                expanded, expandedSize, pixels, size, &layout)) {
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
        image->width = location.width;
        image->height = location.height;
    }
    return 1;
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
        properties = ModManifestFindMaterialProperties(s_modManifest,
                                                       exactId);
    }
    if (properties == NULL &&
        AssetMaterialId(baseId, sizeof(baseId), instance->assetKey,
                        instance->assetSet, material)) {
        properties = ModManifestFindMaterialProperties(s_modManifest,
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
    relativePath = ModManifestFindTexture(s_modManifest, exactId);
    if (relativePath == NULL)
        relativePath = ModManifestFindTexture(s_modManifest, baseId);
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
            ModManifestFindTexture(s_modManifest, exactId) != NULL
                ? exactId : baseId,
            relativePath, image->width, image->height);
    return 1;
fail:
    if (pixels != NULL) SDL_free(pixels);
    if (converted != NULL) SDL_DestroySurface(converted);
    fprintf(stderr, "rage-port: invalid texture override %s\n", fullPath);
    return 0;
}

int ModernAssetsLoadMaterial(const RageRenderMeshInstance *instance,
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
