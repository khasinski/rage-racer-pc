#include "rage/track_asset_identity.h"
#include "native_asset_importer.h"
#include "native_mesh_writer.h"
#include "native_import_stream.h"
#include "native_texture.h"
#include "native_sky.h"
#include "race_view.h"
#include "track_material_page.h"
#include "track_texture_snapshot.h"
#include "sky_panorama_layout.h"

#include <SDL3/SDL.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/render.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/car_model_data.h"
#include "game/asset_index.h"
#include "game/model_stream.h"
#include "game/render_state.h"
#include "game/terrain_internal.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "render/car_paint.h"

enum {
    /* The retail archive exposes 130 renderer asset/set pairs. Keep room for
     * regional or modded discs without evicting a mesh still referenced by a
     * captured frame. */
    RAGE_IMPORT_ENTRY_LIMIT = 256,

    RAGE_IMPORT_VRAM_WIDTH = 1024,
    RAGE_IMPORT_VRAM_HEIGHT = 512,
};




static RageImportedMeshEntry s_entries[RAGE_IMPORT_ENTRY_LIMIT];
static uint32_t s_entryCount;
int NativeAssetImporterMaterialSlot(const RenderMeshInstance *instance,
    uint16_t tpage, uint16_t clut) {
    if (!instance) return -1;
    const RageImportedMeshEntry *entry = ImportFindMesh(s_entries, s_entryCount,
        instance->assetKey, instance->assetSet, instance->assetSource);
    if (!entry) return -1;
    for (uint32_t j = 0; j < entry->materialCount; ++j)
        if (entry->materials[j].tpage == tpage && entry->materials[j].clut == clut &&
            !entry->materials[j].hasWindow) return (int)j;
    return -1;
}

static int s_ready;

static int ImportVisitCourseBank(RageImportedFaceVisitor visitor,
                                  void *context, uint32_t *meshCount) {
    if (!g_RenderState.geometry.courseBank) return 0;
    return ImportVisitCourseModels(g_NativeCourseModels, g_CourseModelCount,
                                   visitor, context, meshCount);
}

static int ImportVisitTerrainBank(RageImportedFaceVisitor visitor,
                                   void *context, uint32_t *meshCount) {
    if (!g_RenderState.geometry.cellTable) return 0;
    return ImportVisitTerrainCells(g_NativeTerrainCells, g_TerrainCellCount,
                                   g_RenderState.geometry.cellFaces,
                                   visitor, context, meshCount);
}

static int ImportVisit(const RenderMeshInstance *instance,
                           RageImportedFaceVisitor visitor, void *context,
                           uint32_t *meshCount) {
    if (instance == NULL || visitor == NULL || meshCount == NULL) return 0;
    switch (instance->assetSet) {
    case RAGE_RENDER_ASSET_MODEL_BANK: {
        /* The requested key must own the imported bank. Selecting another
         * car in the showroom must not cache its geometry under this key. */
        if (instance->assetKey < 10 || instance->assetKey >= 74 ||
            (instance->assetKey - 10) % 2 != 0) return 0;
        s32 slot = FindCarAssetSlot((s32)((instance->assetKey - 10) / 2));
        if (slot < 0) return 0;
        return ImportVisitModelBank(&g_ModelBanks[slot], visitor, context,
                                   meshCount);
    }
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1:
        return ImportVisitModelBank(&g_ModelBanks[1], visitor, context,
                                        meshCount);
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2:
        return ImportVisitModelBank(&g_ModelBanks[2], visitor, context,
                                        meshCount);
    case RAGE_RENDER_ASSET_COURSE:
        return ImportVisitCourseBank(visitor, context, meshCount);
    case RAGE_RENDER_ASSET_TERRAIN:
        return ImportVisitTerrainBank(visitor, context, meshCount);
    }
    return 0;
}

static RageImportedMeshEntry *ImportFindEntry(
    uint32_t assetKey, RenderAssetSet assetSet, RenderAssetSource source) {
    return ImportFindMesh(s_entries, s_entryCount, assetKey, assetSet, source);
}

static int ImportVisitSource(void *context, RageImportedFaceVisitor visitor,
                             void *output, uint32_t *meshCount) {
    return ImportVisit(context, visitor, output, meshCount);
}

static RageImportedMeshEntry *ImportBuildMesh(
    const RenderMeshInstance *instance) {
    if (s_entryCount == RAGE_IMPORT_ENTRY_LIMIT ||
        !ImportBuildMeshEntry(instance, ImportVisitSource, (void *)instance,
                              &s_entries[s_entryCount])) return NULL;
    return &s_entries[s_entryCount];
}

/*
 * One snapshot of video memory, shared by every material decoded from it.
 *
 * Each material used to take its own copy of the whole megabyte, with a
 * drawing stall either side. Four hundred materials meant four hundred
 * megabytes moved and eight hundred stalls, all of it reading the same
 * memory, and each copy landing at whatever moment that material happened to
 * be asked for. That is what made the result depend on where the course's own
 * uploads had got to: a texture captured while a section was still arriving
 * kept whatever was there, for good.
 *
 * A megabyte is nothing to hold on to, so it is held. The snapshot is taken
 * once and retaken when the track's assets change, which is the point at
 * which new artwork has been put in place.
 */
/*
 * The track's texture pages, held whole.
 *
 * The console swapped these pages through a megabyte of video memory a row at
 * a time because it had nowhere else to put them. Nothing here has that
 * problem: the two pages are 224 KB each. So they are built once and kept,
 * and the renderer stops caring which one the game currently has installed.
 *
 * Building them needs no particular moment, because the swap is an exchange
 * and loses nothing: every row is either in video memory or in the shadow the
 * game keeps in the loaded asset, and g_TrackTextureShadowPage says which. A
 * single reading of both therefore yields both pages in full, whenever it is
 * taken.
 *
 * This is what used to go wrong. A material was read straight out of video
 * memory at the moment it was first wanted, so one read while a section
 * change was still moving rows kept half of each page, for good. Leaving the
 * first tunnel on Mythical Coast is a section change.
 */
/* The session owns this snapshot; reconstructed track banks are revision-local.
 * This adapter is the only part that knows live PS1 VRAM and game globals. */
static RageTrackTextureSnapshot s_trackImages;

RageTrackTextureGeneration *NativeAssetImporterRetainTextures(uint64_t revision) {
    if (!s_ready || TrackTextureGenerationRevision(s_trackImages.generation) != revision)
        return NULL;
    return TrackTextureSnapshotRetain(&s_trackImages);
}

static int ImportReadVram(void *context, uint16_t *words, size_t count) {
    RECT rect = {0, 0, RAGE_IMPORT_VRAM_WIDTH, RAGE_IMPORT_VRAM_HEIGHT};
    (void)context;
    if (count != (size_t)RAGE_IMPORT_VRAM_WIDTH * RAGE_IMPORT_VRAM_HEIGHT)
        return 0;
    DrawSync(0);
    StoreImage(&rect, (u_long *)words);
    DrawSync(0);
    return 1;
}

static const uint16_t *ImportVramSnapshot(int requireTrackPages) {
    RageTrackTextureSource source = {
        .read = ImportReadVram,
        .shadowRows = g_TrackTextureShadow,
        .shadowBytes = g_TrackTextureShadow != NULL
            ? sizeof(*g_TrackTextureShadow) * RAGE_TRACK_IMAGE_HEIGHT : 0,
        .shadowPages = g_TrackTextureShadowPage,
        .shadowPageCount = sizeof(g_TrackTextureShadowPage)
    };
    return TrackTextureSnapshotAcquire(&s_trackImages,
        TrackAssetIdentityRevision(), &source,
        requireTrackPages ? (g_TrackTexturePageWanted != 0) : -1);
}

int NativeAssetImporterApplyPlayerMarkings(uint16_t clut, ModernAssetImage *image) {
    uint32_t packed[512] = {0}, paletteStorage[8] = {0};
    const uint16_t *words = (const uint16_t *)packed;
    const uint16_t *palette = (const uint16_t *)paletteStorage;
    RECT rect, paletteRect;
    unsigned x, y, atlasX;
    if (!image || !image->pixels || image->width != 256 || image->height != 256 ||
        image->size != 256u * 256u * 4u) return 0;
    if (clut == 0x7801) {
        rect = (RECT){656, 48, 16, 64};
        atlasX = 64;
    } else if (clut == 0x3bef) {
        rect = (RECT){642, 55, 12, 8};
        atlasX = 8;
    } else return 0;
    paletteRect = (RECT){(clut & 63u) * 16u, clut >> 6, 16, 1};
    DrawSync(0);
    StoreImage(&rect, (u_long *)packed);
    StoreImage(&paletteRect, (u_long *)paletteStorage);
    DrawSync(0);
    for (y = 0; y < (unsigned)rect.h; ++y) {
        for (x = 0; x < (unsigned)rect.w * 4u; ++x) {
            unsigned index = (words[y * rect.w + x / 4u] >> ((x & 3u) * 4u)) & 15u;
            uint8_t *rgba = (uint8_t *)image->pixels +
                (((unsigned)rect.y + y) * 256u + atlasX + x) * 4u;
            TextureColor(palette[index], rgba);
        }
    }
    return 1;
}

static int ImportDecodeTexture(
    const RageImportedTextureKey *texture, uint16_t clut,
    const TextureImage *source, uint8_t **pixelsOut, uint8_t **paintOut) {
    uint8_t *pixels = SDL_malloc(256u * 256u * 4u);
    uint8_t *paint = paintOut ? SDL_malloc(256u * 256u) : NULL;
    if (!pixels || (paintOut && !paint) ||
        !DecodeTexture(texture, clut, source, pixels, 256u * 256u * 4u,
                       paint, paint ? 256u * 256u : 0)) {
        SDL_free(pixels); SDL_free(paint);
        return 0;
    }
    *pixelsOut = pixels;
    if (paintOut) *paintOut = paint;
    return 1;
}

int NativeAssetImporterInit(void) {
    s_ready = 1;
    fprintf(stderr, "rage-port: native asset source=live C importer\n");
    return 1;
}

void NativeAssetImporterShutdown(void) {
    uint32_t index;
    for (index = 0; index < s_entryCount; index++) {
        ImportReleaseEntry(&s_entries[index]);
    }
    memset(s_entries, 0, sizeof(s_entries));
    s_entryCount = 0;
    TrackTextureSnapshotRelease(&s_trackImages);
    s_ready = 0;
}

int NativeAssetImporterReady(void) { return s_ready; }

const RageRuntimeCachedMesh *NativeAssetImporterFind(
    const RenderMeshInstance *instance) {
    RageImportedMeshEntry *entry;
    if (!s_ready || instance == NULL) return NULL;
    entry = ImportFindEntry(instance->assetKey, instance->assetSet, instance->assetSource);
    if (!entry && instance->assetSource == RENDER_ASSET_OWNED) return NULL;
    if (entry != NULL) return &entry->cached;
    entry = ImportBuildMesh(instance);
    if (entry == NULL) return NULL;
    s_entryCount++;
    fprintf(stderr,
            "rage-port: imported native mesh asset=%u set=%u meshes=%u "
            "materials=%u vertices=%u indices=%u decoded_bytes=%zu\n",
            instance->assetKey, (unsigned)instance->assetSet,
            entry->cached.mesh.meshCount, entry->materialCount,
            entry->cached.mesh.vertexCount, entry->cached.mesh.indexCount,
            (entry->cached.ownedVertices != NULL
                ? (size_t)entry->cached.mesh.vertexCount * sizeof(RageRuntimeVertex) : 0) +
            (entry->cached.ownedIndices != NULL
                ? (size_t)entry->cached.mesh.indexCount * sizeof(uint32_t) : 0));
    return &entry->cached;
}

int NativeAssetImporterPrepareCar(uint32_t variant, const CarModelData *model) {
    if (!s_ready || !model || variant >= CAR_MODEL_VARIANT_COUNT) return 0;
    CarModelData *models[CAR_MODEL_VARIANT_COUNT] = {0};
    models[variant] = (CarModelData *)model;
    return ImportPrepareCars(s_entries, &s_entryCount, RAGE_IMPORT_ENTRY_LIMIT, models);
}

int NativeAssetImporterPrepareRaceView(const RaceView *view) {
    return s_ready && view &&
        ImportPrepareCars(s_entries, &s_entryCount, RAGE_IMPORT_ENTRY_LIMIT, view->models);
}

uint32_t NativeAssetImporterMeshCount(void) { return s_entryCount; }

const RageRuntimeCachedMesh *NativeAssetImporterPeek(uint32_t assetKey, RenderAssetSet assetSet, RenderAssetSource source) {
    RageImportedMeshEntry *entry = s_ready ? ImportFindEntry(assetKey, assetSet, source) : NULL;
    return entry != NULL ? &entry->cached : NULL;
}

int NativeAssetImporterLoadMaterial(
    const RenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image) {
    RageImportedMeshEntry *entry;
    RageImportedTextureKey *texture;
    const uint16_t *vram;
    uint16_t clut;
    uint8_t *pixels = NULL, *paint = NULL;
    if (!s_ready || instance == NULL || definition == NULL || image == NULL)
        return 0;
    memset(image, 0, sizeof(*image));
    entry = ImportFindEntry(instance->assetKey, instance->assetSet, instance->assetSource);
    if (entry == NULL) {
        if (NativeAssetImporterFind(instance) == NULL) return 0;
        entry = ImportFindEntry(instance->assetKey, instance->assetSet, instance->assetSource);
    }
    if (entry == NULL || material >= entry->materialCount) return 0;
    texture = &entry->materials[material];
    clut = ImportMaterialClut(texture, instance->assetSet, variant);
    int decoded;
    if (entry->carSource != NULL) {
TextureImage images[3];
if (!CarTextureImages(entry->carSource, images)) return 0;
decoded = ImportDecodeTexture(texture, clut, images, &pixels,
                              instance->hasCarPaint ? &paint : NULL);
    } else {
        vram = ImportVramSnapshot(
            instance->assetSet != RAGE_RENDER_ASSET_MODEL_BANK);
        /* Decode the requested bank, including proactive loads before the game
         * changes its active bank. Never alter live VRAM or the game's globals. */
        if (vram != NULL && (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN ||
                             instance->assetSet == RAGE_RENDER_ASSET_COURSE))
            vram = TrackTextureSnapshotSelectPage(&s_trackImages,
                TrackMaterialPage(instance->assetSet, variant,
                                  g_TrackTexturePageWanted));
        RageTrackTextureGeneration *generation = vram != NULL
            ? TrackTextureSnapshotRetain(&s_trackImages) : NULL;
        const TextureImage source = {vram, RAGE_IMPORT_VRAM_WIDTH * RAGE_IMPORT_VRAM_HEIGHT,
                                     0, 0, RAGE_IMPORT_VRAM_WIDTH, RAGE_IMPORT_VRAM_HEIGHT, NULL};
        decoded = generation != NULL &&
            ImportDecodeTexture(texture, clut, &source, &pixels,
                                instance->hasCarPaint ? &paint : NULL);
        TrackTextureGenerationRelease(generation);
    }
    if (!decoded) {
        SDL_free(pixels);
        SDL_free(paint);
        return 0;
    }
    RenderMaterialDefault(definition);
    if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
        definition->roughness = 0.96f;
    } else if (instance->assetSet == RAGE_RENDER_ASSET_COURSE) {
        definition->roughness = 0.82f;
        if (texture->emissive) {
            definition->shading = RAGE_RENDER_MATERIAL_SHADING_UNLIT;
            definition->emissiveFactor[0] = 0.35f;
            definition->emissiveFactor[1] = 0.28f;
            definition->emissiveFactor[2] = 0.16f;
        }
    } else if (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK) {
        definition->roughness = instance->hasCarPaint ? 0.22f : 0.42f;
        definition->metallic = instance->hasCarPaint ? 0.18f : 0.05f;
    } else {
        definition->roughness = 0.35f;
        definition->metallic = 0.08f;
    }
    if (paint != NULL &&
        !CarPaintApply(pixels, paint, 256u * 256u,
                           instance->carPaintColor1,
                           instance->carPaintColor2)) {
        SDL_free(pixels);
        SDL_free(paint);
        return 0;
    }
    SDL_free(paint);
    image->pixels = pixels;
    image->size = 256u * 256u * 4u;
    image->width = 256;
    image->height = 256;
    return 1;
}

int NativeAssetImporterLoadSky(uint32_t assetKey,
                                   const RageSkyPanoramaLayout *capturedLayout,
                                   ModernAssetImage *image) {
    RageSkyPanoramaLayout layout;
    const uint16_t *vram;
    uint8_t *sky;
    uint32_t pixel;
    uint32_t opaquePixels = 0;
    (void)assetKey;
    if (!s_ready || image == NULL) return 0;
    memset(image, 0, sizeof(*image));
    if (capturedLayout != NULL) layout = *capturedLayout;
    else RageSkyCapturePanoramaLayout(&layout, g_SkyTileMap, g_SkyRowBase);
    for (unsigned r = 0; r < 2; ++r)
        for (unsigned c = 0; c < 8; ++c)
            if (layout.tiles[r][c] >= RAGE_SKY_TILE_COUNT) return 0;
    vram = ImportVramSnapshot(1);
    const TextureImage source = {vram, RAGE_IMPORT_VRAM_WIDTH * RAGE_IMPORT_VRAM_HEIGHT,
                                 0, 0, RAGE_IMPORT_VRAM_WIDTH, RAGE_IMPORT_VRAM_HEIGHT, NULL};
    sky = SDL_malloc(512u * 256u * 4u);
    if (!sky || !vram || !DecodeSky(&source, &layout, sky, 512u * 256u * 4u)) {
        SDL_free(sky);
        return 0;
    }
    for (pixel = 0; pixel < 512u * 256u; ++pixel)
        opaquePixels += sky[pixel * 4u + 3] != 0;
    /* A blank capture happens transiently on some Vulkan/Linux drivers when
     * the course's LoadImage sequence is still in flight. Tell the caller to
     * use its gradient fallback so it can retry rather than cache darkness. */
    if (opaquePixels == 0) {
        SDL_free(sky);
        return 0;
    }
    image->pixels = sky;
    image->size = 512u * 256u * 4u;
    image->width = 512;
    image->height = 256;
    return 1;
}
