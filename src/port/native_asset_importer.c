#include "rage/track_asset_identity.h"
#include "native_asset_importer.h"
#include "native_mesh_writer.h"
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
    RAGE_IMPORT_MATERIAL_LIMIT = 2048,
    RAGE_IMPORT_BATCH_GUARD = 65536,
    RAGE_IMPORT_VRAM_WIDTH = 1024,
    RAGE_IMPORT_VRAM_HEIGHT = 512,
};




typedef int (*RageImportedFaceVisitor)(uint32_t mesh,
                                       const RageImportedFace *face,
                                       void *context);

typedef struct RageImportedScan {
    RageImportedTextureKey *materials;
    uint32_t materialCount;
    uint64_t faceCount;
} RageImportedScan;


static RageImportedMeshEntry s_entries[RAGE_IMPORT_ENTRY_LIMIT];
static uint32_t s_entryCount;
static int s_ready;

static uint16_t ImportRead16(const void *pointer) {
    const uint8_t *p = pointer;
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t ImportRead32(const void *pointer) {
    const uint8_t *p = pointer;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}


static void ImportTextureWindow(uint32_t word,
                                    RageImportedTextureKey *texture) {
    uint32_t value, maskU, maskV, offsetU, offsetV;
    if ((word >> 24) != 0xE2) return;
    value = word & 0xFFFFFu;
    maskU = value & 0x1Fu;
    maskV = (value >> 5) & 0x1Fu;
    if (maskU == 0 && maskV == 0) return;
    offsetU = (value >> 10) & 0x1Fu;
    offsetV = (value >> 15) & 0x1Fu;
    texture->hasWindow = 1;
    texture->windowWidthU = (uint16_t)(256u - maskU * 8u);
    texture->windowWidthV = (uint16_t)(256u - maskV * 8u);
    texture->windowOffsetU = (uint16_t)((offsetU & maskU) * 8u);
    texture->windowOffsetV = (uint16_t)((offsetV & maskV) * 8u);
}

static int ImportVisitModelStream(
    uint32_t mesh, const uint8_t *stream, const SVec *vertices,
    const SVec *normals, RageImportedFaceVisitor visitor, void *context) {
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        const s32 stride = ModelPrimitiveStride(prim);
        if (stride == 0) return 0;
        for (face = 0; face < count; face++, stream += stride) {
            RageImportedFace value;
            uint32_t corner;
            memset(&value, 0, sizeof(value));
            value.vertices = vertices;
            value.normals = normals;
            value.prim = (uint8_t)prim;
            value.depthBias = (int8_t)stream[stride - 3];
            value.color[0] = value.color[1] = value.color[2] = 255;
            for (corner = 0; corner < 4; corner++)
                value.vertex[corner] = ImportRead16(stream + corner * 2);
            if (prim == 0) {
                memcpy(value.color, stream + 8, 3);
            } else if (prim == 1) {
                static const uint8_t offsets[] = {8, 0x0C, 0x10, 0x12};
                value.textured = 1;
                for (corner = 0; corner < 4; corner++) {
                    value.uv[corner][0] = stream[offsets[corner]];
                    value.uv[corner][1] = stream[offsets[corner] + 1];
                }
                value.texture.clut = ImportRead16(stream + 0x0A);
                value.texture.tpage = ImportRead16(stream + 0x0E);
            } else if (prim == 2) {
                value.hasNormals = 1;
                for (corner = 0; corner < 4; corner++)
                    value.normal[corner] =
                        ImportRead16(stream + 8 + corner * 2);
                memcpy(value.color, stream + 0x10, 3);
            } else {
                static const uint8_t offsets[] = {0x10, 0x14, 0x18, 0x1A};
                value.textured = 1;
                value.hasNormals = 1;
                for (corner = 0; corner < 4; corner++) {
                    value.normal[corner] =
                        ImportRead16(stream + 8 + corner * 2);
                    value.uv[corner][0] = stream[offsets[corner]];
                    value.uv[corner][1] = stream[offsets[corner] + 1];
                }
                value.texture.clut = ImportRead16(stream + 0x12);
                value.texture.tpage = ImportRead16(stream + 0x16);
            }
            if (!visitor(mesh, &value, context)) return 0;
        }
    }
    return 0;
}

static int ImportVisitModelBank(const NativeModelBank *bank,
                                    RageImportedFaceVisitor visitor,
                                    void *context, uint32_t *meshCount) {
    uint32_t mesh;
    if (bank == NULL || bank->modelCount <= 0 || bank->table == NULL) return 0;
    *meshCount = (uint32_t)bank->modelCount;
    for (mesh = 0; mesh < *meshCount; mesh++) {
        if (bank->models[mesh] == NULL ||
            !ImportVisitModelStream(mesh, bank->models[mesh], bank->table,
                                        bank->normals, visitor, context))
            return 0;
    }
    return 1;
}

static int ImportVisitCourseStream(
    uint32_t mesh, const uint8_t *stream, const SVec *vertices,
    RageImportedFaceVisitor visitor, void *context) {
    static const uint8_t biasOffsets[] = {0x0D, 0x19, 0x19, 0x19};
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        const s32 stride = CoursePrimitiveStride(prim);
        if (stride == 0) return 0;
        for (face = 0; face < count; face++, stream += stride) {
            RageImportedFace value;
            uint32_t corner;
            static const uint8_t offsets[] = {0x0C, 0x10, 0x14, 0x16};
            memset(&value, 0, sizeof(value));
            value.vertices = vertices;
            value.prim = (uint8_t)prim;
            value.depthBias = (int8_t)stream[biasOffsets[prim]];
            memcpy(value.color, stream + 8, 3);
            for (corner = 0; corner < 4; corner++)
                value.vertex[corner] = ImportRead16(stream + corner * 2);
            if (prim != 0) {
                value.textured = 1;
                for (corner = 0; corner < 4; corner++) {
                    value.uv[corner][0] = stream[offsets[corner]];
                    value.uv[corner][1] = stream[offsets[corner] + 1];
                }
                value.texture.clut = ImportRead16(stream + 0x0E);
                value.texture.tpage = ImportRead16(stream + 0x12);
                if (prim >= 2)
                    ImportTextureWindow(ImportRead32(stream + 0x1C),
                                            &value.texture);
                value.texture.emissive = prim == 3;
            }
            if (!visitor(mesh, &value, context)) return 0;
        }
    }
    return 0;
}

static int ImportVisitCourseBank(RageImportedFaceVisitor visitor,
                                     void *context, uint32_t *meshCount) {
    uint32_t mesh;
    if (g_CourseModelCount <= 0 || g_RenderState.courseBank == NULL) return 0;
    *meshCount = (uint32_t)g_CourseModelCount;
    for (mesh = 0; mesh < *meshCount; mesh++) {
        const NativeCourseModel *model = &g_NativeCourseModels[mesh];
        if (model->geometry == NULL || model->model == NULL ||
            !ImportVisitCourseStream(mesh, model->model, model->geometry,
                                         visitor, context)) return 0;
    }
    return 1;
}

static int ImportVisitTerrainStream(
    uint32_t mesh, const uint8_t *stream, const SVec *vertices,
    RageImportedFaceVisitor visitor, void *context) {
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        const s32 stride = TerrainPrimitiveStride(prim);
        if (stride == 0) return 0;
        for (face = 0; face < count; face++, stream += stride) {
            RageImportedFace value;
            uint32_t corner;
            static const uint8_t offsets[] = {8, 0x0C, 0x10, 0x12};
            memset(&value, 0, sizeof(value));
            value.vertices = vertices;
            value.prim = (uint8_t)prim;
            value.textured = 1;
            value.depthBias = (int8_t)stream[0x15];
            value.flags = stream[0x14];
            memcpy(value.color, stream + 0x1C, 3);
            for (corner = 0; corner < 4; corner++) {
                value.vertex[corner] = ImportRead16(stream + corner * 2);
                value.uv[corner][0] = stream[offsets[corner]];
                value.uv[corner][1] = stream[offsets[corner] + 1];
            }
            value.texture.clut = ImportRead16(stream + 0x0A);
            if (prim >= 2)
                value.texture.clut = (uint16_t)(value.texture.clut +
                                                 ((prim - 2) & 1));
            value.texture.tpage = ImportRead16(stream + 0x0E);
            if (stride == 0x24)
                ImportTextureWindow(ImportRead32(stream + 0x20),
                                        &value.texture);
            if (!visitor(mesh, &value, context)) return 0;
        }
    }
    return 0;
}

static int ImportVisitTerrainBank(RageImportedFaceVisitor visitor,
                                      void *context, uint32_t *meshCount) {
    const SVec *vertices = g_RenderState.cellFaces;
    uint32_t mesh;
    if (g_TerrainCellCount <= 0 || vertices == NULL ||
        g_RenderState.cellTable == NULL) return 0;
    *meshCount = (uint32_t)g_TerrainCellCount;
    for (mesh = 0; mesh < *meshCount; mesh++) {
        if (g_NativeTerrainCells[mesh] == NULL ||
            !ImportVisitTerrainStream(mesh, g_NativeTerrainCells[mesh],
                                          vertices, visitor, context))
            return 0;
    }
    return 1;
}

static int ImportVisit(const RageRenderMeshInstance *instance,
                           RageImportedFaceVisitor visitor, void *context,
                           uint32_t *meshCount) {
    if (instance == NULL || visitor == NULL || meshCount == NULL) return 0;
    switch (instance->assetSet) {
    case RAGE_RENDER_ASSET_MODEL_BANK:
        return ImportVisitModelBank(&g_ModelBanks[0], visitor, context,
                                        meshCount);
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

static int ImportScanFace(uint32_t mesh, const RageImportedFace *face,
                              void *context) {
    RageImportedScan *scan = context;
    uint32_t material;
    (void)mesh;
    if (scan->faceCount == UINT32_MAX / 4u) return 0;
    scan->faceCount++;
    if (!face->textured) return 1;
    for (material = 0; material < scan->materialCount; material++) {
        if (ImportTextureEqual(&scan->materials[material],
                                   &face->texture)) {
            if (face->texture.emissive)
                scan->materials[material].emissive = 1;
            return 1;
        }
    }
    if (scan->materialCount == RAGE_IMPORT_MATERIAL_LIMIT) return 0;
    scan->materials[scan->materialCount++] = face->texture;
    return 1;
}


static RageImportedMeshEntry *ImportFindEntry(
    uint32_t assetKey, RageRenderAssetSet assetSet) {
    uint32_t index;
    for (index = 0; index < s_entryCount; index++)
        if (s_entries[index].cached.assetKey == assetKey &&
            s_entries[index].cached.assetSet == assetSet)
            return &s_entries[index];
    return NULL;
}

static void ImportReleaseMeshBytes(void *context, const void *bytes) {
    (void)context;
    free((void *)bytes);
}

static RageImportedMeshEntry *ImportBuildMesh(
    const RageRenderMeshInstance *instance) {
    RageImportedTextureKey *materials;
    RageImportedScan scan;
    RageImportedWrite write;
    RageImportedMeshEntry *entry;
    uint32_t meshCount;
    RageRuntimeMeshLayout layout;
    uint8_t *bytes;
    if (s_entryCount == RAGE_IMPORT_ENTRY_LIMIT) return NULL;
    materials = calloc(RAGE_IMPORT_MATERIAL_LIMIT, sizeof(*materials));
    if (materials == NULL) return NULL;
    memset(&scan, 0, sizeof(scan));
    scan.materials = materials;
    if (!ImportVisit(instance, ImportScanFace, &scan, &meshCount) ||
        scan.faceCount == 0 || scan.faceCount > UINT32_MAX / 6u ||
        !RuntimeMeshLayout(meshCount, (uint32_t)scan.faceCount * 4u,
                           (uint32_t)scan.faceCount * 6u, &layout)) {
        free(materials);
        return NULL;
    }
    bytes = calloc(1, layout.totalSize);
    if (bytes == NULL) {
        free(materials);
        return NULL;
    }
    if (!RuntimeMeshEncodeHeader(bytes, layout.totalSize, meshCount,
                                (uint32_t)scan.faceCount * 4u,
                                (uint32_t)scan.faceCount * 6u)) {
        free(bytes);
        free(materials);
        return NULL;
    }
    entry = &s_entries[s_entryCount];
    memset(entry, 0, sizeof(*entry));
    entry->cached.assetKey = instance->assetKey;
    entry->cached.assetSet = instance->assetSet;
    entry->materials = materials;
    entry->materialCount = scan.materialCount;
    memset(&write, 0, sizeof(write));
    write.entry = entry;
    write.offsets = bytes + layout.offsetsOffset;
    write.vertexCursor = bytes + layout.verticesOffset;
    write.indexCursor = bytes + layout.indicesOffset;
    write.meshLimit = meshCount;
    write.vertexLimit = (uint32_t)scan.faceCount * 4u;
    write.indexLimit = (uint32_t)scan.faceCount * 6u;
    if (!ImportVisit(instance, ImportWriteFace, &write, &meshCount) ||
        !ImportWriteFinish(&write, meshCount)) {
        free(bytes);
        free(entry->materials);
        memset(entry, 0, sizeof(*entry));
        return NULL;
    }
    if (!RuntimeCachedMeshAdopt(&entry->cached, bytes, layout.totalSize,
                                ImportReleaseMeshBytes, NULL)) {
        free(bytes);
        free(entry->materials);
        memset(entry, 0, sizeof(*entry));
        return NULL;
    }
    return entry;
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

static void ImportColor(uint16_t word, uint8_t rgba[4]) {
    uint8_t r = (uint8_t)((word & 0x1Fu) << 3);
    uint8_t g = (uint8_t)(((word >> 5) & 0x1Fu) << 3);
    uint8_t b = (uint8_t)(((word >> 10) & 0x1Fu) << 3);
    rgba[0] = (uint8_t)(r | (r >> 5));
    rgba[1] = (uint8_t)(g | (g >> 5));
    rgba[2] = (uint8_t)(b | (b >> 5));
    rgba[3] = word == 0 ? 0 : 255;
}

static uint8_t ImportPaletteIndex(const uint16_t *vram, uint16_t tpage,
                                      uint32_t u, uint32_t v) {
    uint32_t pageX = (tpage & 0xFu) * 64u;
    uint32_t pageY = ((tpage >> 4) & 1u) * 256u;
    uint32_t mode = (tpage >> 7) & 3u;
    uint16_t word;
    if (pageY + v >= RAGE_IMPORT_VRAM_HEIGHT) return 0;
    if (mode == 0) {
        if (pageX + u / 4u >= RAGE_IMPORT_VRAM_WIDTH) return 0;
        word = vram[(pageY + v) * RAGE_IMPORT_VRAM_WIDTH + pageX + u / 4u];
        return (uint8_t)((word >> ((u & 3u) * 4u)) & 0xFu);
    }
    if (mode == 1) {
        if (pageX + u / 2u >= RAGE_IMPORT_VRAM_WIDTH) return 0;
        word = vram[(pageY + v) * RAGE_IMPORT_VRAM_WIDTH + pageX + u / 2u];
        return (uint8_t)((word >> ((u & 1u) * 8u)) & 0xFFu);
    }
    return 0;
}

static uint16_t ImportTextureWord(const uint16_t *vram, uint16_t tpage,
                                      uint16_t clut, uint32_t u,
                                      uint32_t v) {
    uint32_t pageX = (tpage & 0xFu) * 64u;
    uint32_t pageY = ((tpage >> 4) & 1u) * 256u;
    uint32_t mode = (tpage >> 7) & 3u;
    uint32_t clutX = (clut & 0x3Fu) * 16u;
    uint32_t clutY = (clut >> 6) & 0x1FFu;
    uint32_t index;
    if (pageY + v >= RAGE_IMPORT_VRAM_HEIGHT) return 0;
    if (mode <= 1) {
        index = ImportPaletteIndex(vram, tpage, u, v);
        if (clutX + index >= RAGE_IMPORT_VRAM_WIDTH ||
            clutY >= RAGE_IMPORT_VRAM_HEIGHT) return 0;
        return vram[clutY * RAGE_IMPORT_VRAM_WIDTH + clutX + index];
    }
    if (pageX + u >= RAGE_IMPORT_VRAM_WIDTH) return 0;
    return vram[(pageY + v) * RAGE_IMPORT_VRAM_WIDTH + pageX + u];
}

static uint8_t ImportCarPaintCode(uint32_t x, uint32_t y) {
    static const uint16_t slots3A[] =
        {1, 0x41, 0xC1, 0x101, 0x181, 0x241, 0x281, 0x301, 0x341};
    static const uint16_t slots3B[] =
        {1, 0x41, 0xC1, 0x181, 0x241, 0x281, 0x301, 0x341};
    static const uint16_t slots4[] = {0x141, 0x1C1, 0x201, 0x401};
    static const uint8_t first3[] = {1, 4, 7};
    static const uint8_t second3[] = {8, 11, 14};
    static const uint8_t first4[] = {1, 3, 5, 7};
    static const uint8_t second4[] = {8, 10, 12, 14};
    static const uint8_t first5[] = {1, 2, 4, 6, 7};
    static const uint8_t second5[] = {8, 9, 11, 13, 14};
    uint32_t word, entry, index;
    if (x < 704 || x >= 768 || y >= 256) return 0;
    word = (y * 64u + (x - 704u));
    if (word < 0x7060u / 2u) return 0;
    entry = word - 0x7060u / 2u;
    for (index = 0; index < sizeof(slots3A) / sizeof(slots3A[0]); index++) {
        uint32_t offset = entry - slots3A[index];
        if (entry >= slots3A[index] && offset < 3) return first3[offset];
    }
    for (index = 0; index < sizeof(slots3B) / sizeof(slots3B[0]); index++) {
        uint32_t start = slots3B[index] + 3u;
        uint32_t offset = entry - start;
        if (entry >= start && offset < 3) return second3[offset];
    }
    for (index = 0; index < sizeof(slots4) / sizeof(slots4[0]); index++) {
        uint32_t start = slots4[index];
        if (entry >= start && entry - start < 4)
            return first4[entry - start];
        start += 4;
        if (entry >= start && entry - start < 4)
            return second4[entry - start];
    }
    if (entry >= 0x2C1 && entry - 0x2C1 < 5)
        return first5[entry - 0x2C1];
    if (entry >= 0x2C6 && entry - 0x2C6 < 5)
        return second5[entry - 0x2C6];
    return 0;
}

static int ImportDecodeTexture(
    const RageImportedTextureKey *texture, uint16_t clut,
    const uint16_t *vram, uint8_t **pixelsOut, uint8_t **paintOut) {
    uint8_t *pixels = SDL_malloc(256u * 256u * 4u);
    uint8_t *paint = paintOut != NULL ? SDL_calloc(256u * 256u, 1) : NULL;
    uint32_t y, x;
    if (pixels == NULL || (paintOut != NULL && paint == NULL)) {
        SDL_free(pixels);
        SDL_free(paint);
        return 0;
    }
    for (y = 0; y < 256; y++) {
        uint32_t sourceV = texture->hasWindow
            ? y % texture->windowWidthV + texture->windowOffsetV : y;
        for (x = 0; x < 256; x++) {
            uint32_t sourceU = texture->hasWindow
                ? x % texture->windowWidthU + texture->windowOffsetU : x;
            uint8_t *rgba = pixels + (y * 256u + x) * 4u;
            ImportColor(ImportTextureWord(
                                vram, texture->tpage, clut, sourceU, sourceV),
                            rgba);
            if (paint != NULL && ((texture->tpage >> 7) & 3u) <= 1) {
                uint32_t clutX = (clut & 0x3Fu) * 16u;
                uint32_t clutY = (clut >> 6) & 0x1FFu;
                uint8_t palette = ImportPaletteIndex(
                    vram, texture->tpage, sourceU, sourceV);
                paint[y * 256u + x] =
                    ImportCarPaintCode(clutX + palette, clutY);
            }
        }
    }
    *pixelsOut = pixels;
    if (paintOut != NULL) *paintOut = paint;
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
        RuntimeCachedMeshRelease(&s_entries[index].cached);
        free(s_entries[index].materials);
    }
    memset(s_entries, 0, sizeof(s_entries));
    s_entryCount = 0;
    TrackTextureSnapshotRelease(&s_trackImages);
    s_ready = 0;
}

int NativeAssetImporterReady(void) { return s_ready; }

const RageRuntimeCachedMesh *NativeAssetImporterFind(
    const RageRenderMeshInstance *instance) {
    RageImportedMeshEntry *entry;
    if (!s_ready || instance == NULL) return NULL;
    entry = ImportFindEntry(instance->assetKey, instance->assetSet);
    if (entry != NULL) return &entry->cached;
    entry = ImportBuildMesh(instance);
    if (entry == NULL) return NULL;
    s_entryCount++;
    fprintf(stderr,
            "rage-port: imported native mesh asset=%u set=%u meshes=%u "
            "materials=%u\n",
            instance->assetKey, (unsigned)instance->assetSet,
            entry->cached.mesh.meshCount, entry->materialCount);
    return &entry->cached;
}

uint32_t NativeAssetImporterMeshCount(void) { return s_entryCount; }

const RageRuntimeCachedMesh *NativeAssetImporterPeek(uint32_t assetKey, RageRenderAssetSet assetSet) {
    RageImportedMeshEntry *entry = s_ready ? ImportFindEntry(assetKey, assetSet) : NULL;
    return entry != NULL ? &entry->cached : NULL;
}

int NativeAssetImporterLoadMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image) {
    RageImportedMeshEntry *entry;
    RageImportedTextureKey *texture;
    const uint16_t *vram;
    uint16_t clut;
    uint8_t *pixels = NULL, *paint = NULL;
    uint32_t clutOffset = 0;
    if (!s_ready || instance == NULL || definition == NULL || image == NULL)
        return 0;
    memset(image, 0, sizeof(*image));
    entry = ImportFindEntry(instance->assetKey, instance->assetSet);
    if (entry == NULL) {
        if (NativeAssetImporterFind(instance) == NULL) return 0;
        entry = ImportFindEntry(instance->assetKey, instance->assetSet);
    }
    if (entry == NULL || material >= entry->materialCount) return 0;
    texture = &entry->materials[material];
    if (instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)
        clutOffset = variant % 3u;
    else if (instance->assetSet == RAGE_RENDER_ASSET_COURSE)
        clutOffset = variant % 4u;
    else if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN)
        clutOffset = variant % 2u;
    clut = (uint16_t)(texture->clut + clutOffset);
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
    int decoded = generation != NULL &&
        ImportDecodeTexture(texture, clut, vram, &pixels,
                            instance->hasCarPaint ? &paint : NULL);
    TrackTextureGenerationRelease(generation);
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
    RageImportedTextureKey texture;
    const uint16_t *vram;
    uint8_t *page = NULL;
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
    memset(&texture, 0, sizeof(texture));
    texture.tpage = 0x18;
    texture.clut = 0x798E;
    vram = ImportVramSnapshot(1);
    if (vram == NULL ||
        !ImportDecodeTexture(&texture, texture.clut, vram, &page, NULL)) {
        SDL_free(page);
        return 0;
    }
    sky = SDL_calloc(512u * 256u, 4u);
    if (sky == NULL) {
        SDL_free(page);
        return 0;
    }
    if (!RageSkyExpandTexturePageLayout(sky, 512u * 256u * 4u,
                                        page, 256u * 256u * 4u, &layout)) {
        SDL_free(page);
        SDL_free(sky);
        return 0;
    }
    SDL_free(page);
    for (pixel = 0; pixel < 512u * 256u; pixel++) {
        uint8_t *rgba = sky + pixel * 4u;
        uint8_t brightness = rgba[0];
        if (rgba[1] > brightness) brightness = rgba[1];
        if (rgba[2] > brightness) brightness = rgba[2];
        if (rgba[3] != 0) {
            rgba[0] = rgba[1] = rgba[2] = brightness;
            opaquePixels++;
        } else {
            rgba[0] = rgba[1] = rgba[2] = 0;
        }
    }
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
