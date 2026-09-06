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
#include "render/authored_car_surface.h"
#include "render/mod_manifest.h"
#include "render/resource_provider.h"
#include "modern_material_transaction.h"
#include "render/rmesh_replace.h"
#include "authored_car_data.h"

/* One entry per bank's first definition. More than one car may share a bank;
 * all matching replacements are assembled before the result is cached. */
static RageRuntimeCachedMesh
    s_authoredCarMesh[RAGE_AUTHORED_CAR_COUNT ? RAGE_AUTHORED_CAR_COUNT : 1];
static char s_modRoot[1024];
static const RageModManifest *s_modManifest;
static int s_modReady;

static void AuthoredCarKey(const AuthoredCarReplacement *car, char *key, size_t size) {
    snprintf(key, size, "car.%s.%u.part.%u",
             car->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? "player" : "rival",
             (unsigned)car->assetKey, (unsigned)car->submesh);
}

int ModernAssetsCarCatalog(void) {
    size_t i;
    puts("{\"parts\":[");
    for (i = 0; i != RAGE_AUTHORED_CAR_COUNT; i++) {
        const AuthoredCarReplacement *car = &s_authoredCars[i];
        RageRuntimeMesh mesh;
        char key[96];
        const unsigned char *name = (const unsigned char *)car->name;
        if (!RuntimeMeshOpen(&mesh, car->bytes, car->byteCount)) return 0;
        AuthoredCarKey(car, key, sizeof(key));
        printf("%s{\"key\":\"%s\",\"name\":\"", i ? "," : "", key);
        for (; *name; name++) {
            if (*name == '"' || *name == '\\') putchar('\\');
            if (*name < 32 || *name >= 127) printf("\\u%04x", *name);
            else putchar(*name);
        }
        printf("\",\"bank\":%u,\"part\":%u,\"rival\":%s,\"vertices\":%u,\"triangles\":%u}",
               (unsigned)car->assetKey, (unsigned)car->submesh,
               car->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? "false" : "true",
               mesh.vertexCount, mesh.indexCount / 3);
    }
    puts("]}");
    return !ferror(stdout);
}

int ModernAssetsExportCar(const char *key, const char *path) {
    size_t i;
    if (!key || !path || !*path) return 0;
    for (i = 0; i != RAGE_AUTHORED_CAR_COUNT; i++) {
        const AuthoredCarReplacement *car = &s_authoredCars[i];
        char candidate[96];
        FILE *file;
        int ok;
        AuthoredCarKey(car, candidate, sizeof(candidate));
        if (strcmp(key, candidate)) continue;
        file = fopen(path, "wb");
        if (!file) return 0;
        ok = fwrite(car->bytes, 1, car->byteCount, file) == car->byteCount;
        if (fclose(file)) ok = 0;
        if (!ok) remove(path);
        if (ok) {
            size_t m;
            printf("{\"materials\":[");
            for (m = 0; m < car->materialCount; m++) {
                const AuthoredCarMaterial *material = &car->materials[m];
                printf("%s{\"source\":%u,\"page\":%u,\"clut\":%u}",
                       m ? "," : "", material->source, material->page, material->clut);
            }
            puts("]}");
        }
        return ok;
    }
    fprintf(stderr, "rage-port: unknown car mesh %s\n", key);
    return 0;
}

static int AuthoredCarMatches(const AuthoredCarReplacement *car,
                             const RageRenderMeshInstance *instance) {
    return car->assetKey == instance->assetKey &&
           car->assetSet == instance->assetSet;
}

static void ReleaseAuthoredBytes(void *context, const void *bytes) {
    (void)context;
    free((void *)bytes);
}

static const RageRuntimeCachedMesh *ModernAuthoredCar(
    const RageRuntimeCachedMesh *base, const RageRenderMeshInstance *instance,
    int imported) {
    RageRuntimeMesh working;
    RageRuntimeCachedMesh *entry;
    size_t ownedSize = 0;
    void *owned = NULL;
    size_t first, i;
    int authored = RuntimeConfigInt("modern.authored_cars", 1, 0, 1);
    if (!base) return NULL;
    if (instance->assetSet != RAGE_RENDER_ASSET_MODEL_BANK &&
        instance->assetSet != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) return base;
    for (first = 0; first != RAGE_AUTHORED_CAR_COUNT; first++)
        if (AuthoredCarMatches(&s_authoredCars[first], instance)) break;
    if (first == RAGE_AUTHORED_CAR_COUNT) return base;
    entry = &s_authoredCarMesh[first];
    if (entry->ownedBytes) return entry;
    working = base->mesh;
    for (i = first; i != RAGE_AUTHORED_CAR_COUNT; i++) {
        const AuthoredCarReplacement *car = &s_authoredCars[i];
        RageRuntimeMesh body, next;
        uint32_t map[RAGE_CAR_SURFACE_SOURCE_STRIDE * RAGE_CAR_SURFACE_COUNT];
        size_t j, size;
        void *bytes;
        void *modBytes = NULL;
        const char *override;
        char key[96], fullPath[1600];
        if (!AuthoredCarMatches(car, instance)) continue;
        AuthoredCarKey(car, key, sizeof(key));
        override = s_modReady ? ModManifestFindMesh(s_modManifest, key) : NULL;
        if (!authored && !override) continue;
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
        if (override) {
            size_t modSize;
            snprintf(fullPath, sizeof(fullPath), "%s/%s", s_modRoot, override);
            modBytes = SDL_LoadFile(fullPath, &modSize);
            if (!modBytes || !RuntimeMeshOpen(&body, modBytes, modSize) ||
                body.meshCount != 1) {
                fprintf(stderr, "rage-port: invalid car mesh override %s: %s\n", key, fullPath);
                SDL_free(modBytes);
                goto failed;
            }
        } else if (!RuntimeMeshOpen(&body, car->bytes, car->byteCount)) goto failed;
        bytes = RuntimeMeshReplace(&working, car->submesh, &body,
                                   map, sizeof(map)/sizeof(map[0]), &size);
        SDL_free(modBytes);
        if (!bytes) goto failed;
        if (!RuntimeMeshOpen(&next, bytes, size)) {
            free(bytes);
            goto failed;
        }
        free(owned);
        owned = bytes;
        working = next;
        ownedSize = size;
        if (override)
            fprintf(stderr, "rage-port: car mesh override %s <- %s (%u triangles)\n",
                    key, fullPath, body.indexCount / 3);
        fprintf(stderr, "rage-port: authored %s %s %s installed asset=%u (%u triangles)\n",
                car->name, car->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? "player" : "rival",
                (car->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? car->submesh == 0 : car->submesh % 5 == 0) ? "body" : "wheel",
                car->assetKey, body.indexCount/3);
    }
    if (!owned) return base;
    if (!RuntimeCachedMeshAdopt(entry, owned, ownedSize, ReleaseAuthoredBytes, NULL))
        goto failed;
    entry->assetKey = base->assetKey;
    entry->assetSet = base->assetSet;
    entry->location = base->location;
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
/* Material sidecars are read synchronously on the render thread. Copy the
 * selected relative path out before releasing their transient file buffer. */

static void ModernAssetsInitModProvider(void) {
    const char *root = ModAssetsDirectory();
    size_t rootLength;
    s_modRoot[0] = '\0';
    s_modManifest = NULL;
    s_modReady = 0;
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
    if (s_initialized) return s_ready;
    ModernAssetsInitModProvider();
    configured = RuntimeConfigGetForced("modern.assets");
    /* Normal startup always follows the selected disc. A prebuilt cache has
     * no verified disc/importer identity, so it is an explicit developer
     * override only, never an implicitly discovered source. */
    if (configured != NULL && strcmp(configured, "disc") == 0) {
        configured = NULL;
    } else if (configured != NULL && configured[0] != '\0') {
        if (ModernAssetsTryRoot(configured)) {
            s_initialized = 1;
            return 1;
        }
        fprintf(stderr, "rage-port: native asset cache unavailable: %s\n",
                configured);
        return 0;
    }
    if (NativeAssetImporterReady()) {
        s_importerSource = 1;
        s_ready = 1;
        s_initialized = 1;
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
    ModernAssetsInitModProvider();
    if (ModernAssetsTryRoot(root)) {
        s_initialized = 1;
        return 1;
    }
    fprintf(stderr, "rage-port: native asset cache unavailable: %s\n",
            root != NULL ? root : "(null)");
    return 0;
}

void ModernAssetsShutdown(void) {
    size_t i;
    for (i=0;i<sizeof(s_authoredCarMesh)/sizeof(s_authoredCarMesh[0]);i++)
        RuntimeCachedMeshRelease(&s_authoredCarMesh[i]);
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
    s_modManifest = NULL;
    s_modReady = 0;
    /* Full asset-session shutdown only. Presentation/device restart must keep
     * the manifest alive together with the meshes that were derived from it. */
    ModAssetsShutdown();
}

typedef struct MeshProviderRequest {
    const RageRenderMeshInstance *instance;
    const RageRuntimeCachedMesh *mesh;
    int residentOnly;
} MeshProviderRequest;
static RageResourceStatus ResolveImportedMesh(void *context) {
    MeshProviderRequest *request=context;
    if(!s_importerSource)return RAGE_RESOURCE_MISSING;
    request->mesh=request->residentOnly
        ? NativeAssetImporterPeek(request->instance->assetKey,request->instance->assetSet)
        : NativeAssetImporterFind(request->instance);
    return request->mesh?RAGE_RESOURCE_READY:RAGE_RESOURCE_ERROR;
}
static RageResourceStatus ResolveCachedMesh(void *context) {
    MeshProviderRequest *request=context;
    if(s_importerSource)return RAGE_RESOURCE_MISSING;
    request->mesh=request->residentOnly
        ? RuntimeMeshCachePeek(&s_cache,request->instance->assetKey,request->instance->assetSet)
        : RuntimeMeshCacheFind(&s_cache,request->instance->assetKey,request->instance->assetSet);
    return request->mesh?RAGE_RESOURCE_READY:RAGE_RESOURCE_ERROR;
}
static const RageRuntimeCachedMesh *ResolveBaseMesh(const RageRenderMeshInstance *instance,int residentOnly) {
    MeshProviderRequest request={instance,NULL,residentOnly};
    const RageResourceProvider providers[]={{ResolveImportedMesh,&request},{ResolveCachedMesh,&request}};
    if(ResourceProviderResolve(providers,2,NULL)!=RAGE_RESOURCE_READY)return NULL;
    return request.mesh;
}
const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance) {
    if (!s_ready || instance == NULL) return NULL;
    return ModernAuthoredCar(ResolveBaseMesh(instance,0),instance,s_importerSource);
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

const RageRuntimeMesh *ModernAssetsResidentMeshLookup(
    void *context, const RageRenderMeshInstance *instance) {
    (void)context;
    if (!s_ready || instance == NULL) return NULL;
    for (size_t i = 0; i != RAGE_AUTHORED_CAR_COUNT; ++i) {
        if (AuthoredCarMatches(&s_authoredCars[i], instance) &&
            s_authoredCarMesh[i].ownedBytes != NULL)
            return &s_authoredCarMesh[i].mesh;
    }
    const RageRuntimeCachedMesh *cached = ResolveBaseMesh(instance,1);
    return cached != NULL ? &cached->mesh : NULL;
}

static const char *ModernAssetsFindModMaterialProperties(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant) {
    char exactId[160], baseId[160];
    if (!s_modReady || !AssetMaterialVariantId(
            exactId, sizeof(exactId), instance->assetKey,
            instance->assetSet, material, variant) ||
        !AssetMaterialId(baseId, sizeof(baseId), instance->assetKey,
                        instance->assetSet, material)) return NULL;
    RageModResolution resolved = ModManifestResolve(s_modManifest, exactId, baseId, RAGE_MOD_RESOLVE_MATERIAL);
    return resolved.material != NULL ? resolved.material->properties : NULL;
}

static int ModernAssetsFindMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition,RageRenderMaterialStorage *storage) {
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
    parsed=parsed&&definition->baseColorTexture.length!=0&&RenderMaterialStorePaths(definition,storage);
    ModernAssetFreeFile(NULL, mapBytes);
    if (!parsed) return 0;
    if (RuntimeConfigEnabled("diagnostics.modern_asset_trace")) {
        fprintf(stderr,
                "rage-port: native material asset=%u set=%u material=%u "
                "variant=%u path=%s\n",
                instance->assetKey, (unsigned)instance->assetSet, material,
                variant, storage->baseColorTexture);
    }
    return 1;
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
    RageModResolution resolved = ModManifestResolve(s_modManifest, exactId, baseId, RAGE_MOD_RESOLVE_TEXTURE);
    relativePath = resolved.texture != NULL ? resolved.texture->path : NULL;
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
            resolved.texture->key,
            relativePath, image->width, image->height);
    return 1;
fail:
    if (pixels != NULL) SDL_free(pixels);
    if (converted != NULL) SDL_DestroySurface(converted);
    fprintf(stderr, "rage-port: invalid texture override %s\n", fullPath);
    return 0;
}

static int ModernAssetsLoadCachedImage(const RageRenderMeshInstance *instance,
                             uint32_t material,
                             RageRenderMaterial *definition,
                             ModernAssetImage *image) {
    const char *path, *paintPath;
    const void *pixels = NULL;
    size_t pathLength, paintPathLength;
    path = definition->baseColorTexture.text;
    pathLength = definition->baseColorTexture.length;
    paintPath = definition->paintMask.text;
    paintPathLength = definition->paintMask.length;
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

typedef struct MaterialProviderRequest {
    const RageRenderMeshInstance *instance;
    uint32_t material;
    uint8_t variant;
    RageRenderMaterial *definition;
    ModernAssetImage *image;
    RageRenderMaterialStorage *storage;
} MaterialProviderRequest;
static RageResourceStatus ResolveImportedMaterial(void *context) {
    MaterialProviderRequest *request=context;
    if(!s_importerSource)return RAGE_RESOURCE_MISSING;
    return NativeAssetImporterLoadMaterial(request->instance,request->material,
        request->variant,request->definition,request->image)
        ? RAGE_RESOURCE_READY : RAGE_RESOURCE_ERROR;
}
static RageResourceStatus ResolveCachedMaterial(void *context) {
    MaterialProviderRequest *request=context;
    if(s_importerSource)return RAGE_RESOURCE_MISSING;
    /* Defer cached pixel I/O until after the mod image has had first choice. */
    return ModernAssetsFindMaterial(request->instance,request->material,
        request->variant,request->definition,request->storage)?RAGE_RESOURCE_READY:RAGE_RESOURCE_ERROR;
}
static RageResourceStatus ResolveModMaterialImage(void *context) {
    MaterialProviderRequest *request=context;
    ModernAssetImage replacement={0};
    if(!ModernAssetsLoadModImage(request->instance,request->material,request->variant,&replacement))
        return RAGE_RESOURCE_MISSING; /* Preserve legacy rejected-image fallback. */
    ModernAssetsFreeMaterialImage(request->image);
    *request->image=replacement;
    return RAGE_RESOURCE_READY;
}
static RageResourceStatus ResolveBaseMaterialImage(void *context) {
    MaterialProviderRequest *request=context;
    if(s_importerSource)return RAGE_RESOURCE_READY; /* Importer already supplied pixels. */
    return ModernAssetsLoadCachedImage(request->instance,request->material,
        request->definition,request->image)?RAGE_RESOURCE_READY:RAGE_RESOURCE_ERROR;
}
static int ModernAssetsLoadBaseMaterial(const RageRenderMeshInstance *instance,
                             uint32_t material,uint8_t variant,
                             RageRenderMaterial *definition,ModernAssetImage *image,RageRenderMaterialStorage *storage) {
    if(!instance||!definition||!image||!storage)return 0;
    memset(image,0,sizeof(*image));
    MaterialProviderRequest request={instance,material,variant,definition,image,storage};
    const RageResourceProvider definitions[]={{ResolveImportedMaterial,&request},{ResolveCachedMaterial,&request}};
    if(ResourceProviderResolve(definitions,2,NULL)!=RAGE_RESOURCE_READY)return 0;
    const RageResourceProvider images[]={{ResolveModMaterialImage,&request},{ResolveBaseMaterialImage,&request}};
    return ResourceProviderResolve(images,2,NULL)==RAGE_RESOURCE_READY;
}

static uint16_t ModernPlayerMarkingClut(const RageRenderMeshInstance *instance,
                                      uint32_t slot) {
    size_t i, j;
    if (instance->assetSet != RAGE_RENDER_ASSET_MODEL_BANK) return 0;
    for (i = 0; i != RAGE_AUTHORED_CAR_COUNT; ++i) {
        const AuthoredCarReplacement *car = &s_authoredCars[i];
        if (!AuthoredCarMatches(car, instance)) continue;
        for (j = 0; j < car->materialCount; ++j) {
            const AuthoredCarMaterial *m = &car->materials[j];
            int resolved;
            if (m->page != 10 || (m->clut != 0x3bef && m->clut != 0x7801))
                continue;
            resolved = s_importerSource ? NativeAssetImporterMaterialSlot(
                instance, m->page, m->clut) : m->cacheSlot;
            if (resolved >= 0 && (uint32_t)resolved == slot) return m->clut;
        }
    }
    return 0;
}

static int ModernAssetsBuildMaterial(const RageRenderMeshInstance *instance,
                             uint32_t material, uint8_t variant,
                             RageRenderMaterial *definition,
                             ModernAssetImage *image,RageRenderMaterialStorage *storage) {
    unsigned surface = 0;
    if (instance && (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
                     instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)) {
        surface = material / RAGE_CAR_SURFACE_RUNTIME_STRIDE;
        if (surface >= RAGE_CAR_SURFACE_COUNT) return 0;
        material %= RAGE_CAR_SURFACE_RUNTIME_STRIDE;
    }
    if (!ModernAssetsLoadBaseMaterial(instance, material, variant, definition, image,storage))
        return 0;
    if(!ModernAssetImageValidRGBA(image)) {
        ModernAssetsFreeMaterialImage(image);return 0;
    }
    if (surface == RAGE_CAR_SURFACE_GLASS || surface == RAGE_CAR_SURFACE_DECAL) {
        uint16_t clut = ModernPlayerMarkingClut(instance, material);
        if (clut && !NativeAssetImporterApplyPlayerMarkings(clut, image)) {
            ModernAssetsFreeMaterialImage(image);
            return 0;
        }
    }
    if (!AuthoredCarSurfaceResolve(surface,
            ModernAssetsFindModMaterialProperties(instance, material, variant),
            definition)) {
        ModernAssetsFreeMaterialImage(image);
        return 0;
    }
    AuthoredCarSurfaceTexture(surface, image->pixels, image->size);
    return 1;
}

typedef struct MaterialBuildRequest {
    const RageRenderMeshInstance *instance;
    uint32_t material;
    uint8_t variant;
} MaterialBuildRequest;
static int BuildMaterialTransaction(void *context,RageRenderMaterial *definition,
                                     ModernAssetImage *image,RageRenderMaterialStorage *storage) {
    MaterialBuildRequest *request=context;
    return ModernAssetsBuildMaterial(request->instance,request->material,request->variant,
                                     definition,image,storage);
}
int ModernAssetsLoadMaterial(const RageRenderMeshInstance *instance,
                             uint32_t material,uint8_t variant,
                             RageRenderMaterial *definition,
                             ModernAssetImage *image,RageRenderMaterialStorage *storage) {
    if(!instance)return 0;
    MaterialBuildRequest request={instance,material,variant};
    /* Publish only after all providers, surface effects and path copies pass.
     * A failed build never exposes borrowed sidecar pointers or partial images. */
    return ModernMaterialTransaction(BuildMaterialTransaction,&request,ModernAssetsFreeMaterialImage,
                                      definition,image,storage);
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
