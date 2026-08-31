#include "rage/track_asset_identity.h"
#include "native_asset_importer.h"

#include <SDL3/SDL.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/render.h"
#include "game/asset.h"
#include "game/scratchpad.h"
#include "game/track.h"
#include "render/car_paint.h"

enum {
    /* The retail archive exposes 130 renderer asset/set pairs. Keep room for
     * regional or modded discs without evicting a mesh still referenced by a
     * captured frame. */
    RAGE_IMPORT_ENTRY_LIMIT = 256,
    RAGE_IMPORT_MATERIAL_LIMIT = 2048,
    RAGE_IMPORT_BATCH_GUARD = 65536,
    RAGE_IMPORT_HEADER_SIZE = 24,
    RAGE_IMPORT_VERTEX_SIZE = 40,
    RAGE_IMPORT_VRAM_WIDTH = 1024,
    RAGE_IMPORT_VRAM_HEIGHT = 512,
};

typedef struct RageImportedTextureKey {
    uint16_t tpage;
    uint16_t clut;
    uint16_t windowWidthU;
    uint16_t windowWidthV;
    uint16_t windowOffsetU;
    uint16_t windowOffsetV;
    uint8_t hasWindow;
    uint8_t emissive;
} RageImportedTextureKey;

typedef struct RageImportedMeshEntry {
    RageRuntimeCachedMesh cached;
    RageImportedTextureKey *materials;
    uint32_t materialCount;
    void *bytes;
} RageImportedMeshEntry;

typedef struct RageImportedFace {
    const SVec *vertices;
    const SVec *normals;
    uint16_t vertex[4];
    uint16_t normal[4];
    uint8_t uv[4][2];
    uint8_t color[3];
    RageImportedTextureKey texture;
    int8_t depthBias;
    uint8_t prim;
    uint8_t flags;
    uint8_t textured;
    uint8_t hasNormals;
} RageImportedFace;

typedef int (*RageImportedFaceVisitor)(uint32_t mesh,
                                       const RageImportedFace *face,
                                       void *context);

typedef struct RageImportedScan {
    RageImportedTextureKey *materials;
    uint32_t materialCount;
    uint64_t faceCount;
} RageImportedScan;

typedef struct RageImportedWrite {
    RageImportedMeshEntry *entry;
    uint8_t *vertexCursor;
    uint8_t *indexCursor;
    uint32_t vertexCount;
    uint32_t indexCount;
} RageImportedWrite;

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

static void ImportWrite32(void *pointer, uint32_t value) {
    uint8_t *p = pointer;
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static int ImportSizeAdd(size_t left, size_t right, size_t *result) {
    if (right > SIZE_MAX - left) return 0;
    *result = left + right;
    return 1;
}

static int ImportSizeMultiply(size_t left, size_t right, size_t *result) {
    if (right != 0 && left > SIZE_MAX / right) return 0;
    *result = left * right;
    return 1;
}

static int ImportTextureEqual(const RageImportedTextureKey *left,
                                  const RageImportedTextureKey *right) {
    return left->tpage == right->tpage && left->clut == right->clut &&
           left->hasWindow == right->hasWindow &&
           (!left->hasWindow ||
            (left->windowWidthU == right->windowWidthU &&
             left->windowWidthV == right->windowWidthV &&
             left->windowOffsetU == right->windowOffsetU &&
             left->windowOffsetV == right->windowOffsetV));
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
    static const uint8_t strides[] = {0x10, 0x18, 0x18, 0x20};
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        if (prim >= sizeof(strides)) return 0;
        for (face = 0; face < count; face++, stream += strides[prim]) {
            RageImportedFace value;
            uint32_t corner;
            memset(&value, 0, sizeof(value));
            value.vertices = vertices;
            value.normals = normals;
            value.prim = (uint8_t)prim;
            value.depthBias = (int8_t)stream[strides[prim] - 3];
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
    static const uint8_t strides[] = {0x10, 0x1C, 0x20, 0x20};
    static const uint8_t biasOffsets[] = {0x0D, 0x19, 0x19, 0x19};
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        if (prim >= sizeof(strides)) return 0;
        for (face = 0; face < count; face++, stream += strides[prim]) {
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
    if (g_CourseModelCount <= 0 || SCRATCH_COURSE_BANK == NULL) return 0;
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
    static const uint8_t strides[] = {0x20, 0x24, 0x20, 0x20, 0x24, 0x24};
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        if (prim >= sizeof(strides)) return 0;
        for (face = 0; face < count; face++, stream += strides[prim]) {
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
            if (strides[prim] == 0x24)
                ImportTextureWindow(ImportRead32(stream + 0x20),
                                        &value.texture);
            if (!visitor(mesh, &value, context)) return 0;
        }
    }
    return 0;
}

static int ImportVisitTerrainBank(RageImportedFaceVisitor visitor,
                                      void *context, uint32_t *meshCount) {
    const SVec *vertices = SCRATCH_CELL_FACES;
    uint32_t mesh;
    if (g_TerrainCellCount <= 0 || vertices == NULL ||
        SCRATCH_CELL_TABLE == NULL) return 0;
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

static uint32_t ImportMaterialIndex(const RageImportedMeshEntry *entry,
                                        const RageImportedFace *face) {
    uint32_t material;
    if (!face->textured) return UINT32_MAX;
    for (material = 0; material < entry->materialCount; material++)
        if (ImportTextureEqual(&entry->materials[material],
                                   &face->texture)) return material;
    return UINT32_MAX;
}

static void ImportWriteFloat(uint8_t *pointer, float value) {
    memcpy(pointer, &value, sizeof(value));
}

static int ImportWriteFace(uint32_t mesh, const RageImportedFace *face,
                               void *context) {
    RageImportedWrite *write = context;
    uint32_t material = ImportMaterialIndex(write->entry, face);
    uint32_t encodedMaterial = material;
    uint32_t corner;
    static const uint32_t order[] = {0, 2, 1, 1, 2, 3};
    (void)mesh;
    if (face->depthBias != 0 ||
        (write->entry->cached.assetSet == RAGE_RENDER_ASSET_TERRAIN &&
         ((face->flags & 2) != 0 || face->prim < 2))) {
        uint32_t materialIndex =
            material == UINT32_MAX ? 0xFFFFu : material;
        encodedMaterial = materialIndex | RAGE_RUNTIME_MATERIAL_METADATA |
            ((uint32_t)(uint8_t)face->depthBias <<
             RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT);
        if (write->entry->cached.assetSet == RAGE_RENDER_ASSET_TERRAIN &&
            (face->flags & 2) != 0)
            encodedMaterial |= RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY;
        if (write->entry->cached.assetSet == RAGE_RENDER_ASSET_TERRAIN &&
            face->prim < 2)
            encodedMaterial |= RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT;
    }
    if (write->entry->cached.assetSet == RAGE_RENDER_ASSET_COURSE &&
        face->prim == 3 && material != UINT32_MAX)
        encodedMaterial |= RAGE_RUNTIME_MATERIAL_SCROLL_U;
    for (corner = 0; corner < 4; corner++) {
        const SVec *position = &face->vertices[face->vertex[corner]];
        const SVec *normal = face->hasNormals
            ? &face->normals[face->normal[corner]] : NULL;
        uint8_t *vertex = write->vertexCursor;
        ImportWriteFloat(vertex + 0, (float)position->vx);
        ImportWriteFloat(vertex + 4, (float)-position->vy);
        ImportWriteFloat(vertex + 8, (float)-position->vz);
        ImportWriteFloat(vertex + 12,
                             normal != NULL ? (float)normal->vx : 0.0f);
        ImportWriteFloat(vertex + 16,
                             normal != NULL ? (float)-normal->vy : 1.0f);
        ImportWriteFloat(vertex + 20,
                             normal != NULL ? (float)-normal->vz : 0.0f);
        vertex[24] = face->color[0];
        vertex[25] = face->color[1];
        vertex[26] = face->color[2];
        vertex[27] = 255;
        ImportWriteFloat(vertex + 28,
            face->textured ? ((float)face->uv[corner][0] + 0.5f) / 256.0f
                           : 0.0f);
        ImportWriteFloat(vertex + 32,
            face->textured ? ((float)face->uv[corner][1] + 0.5f) / 256.0f
                           : 0.0f);
        ImportWrite32(vertex + 36, encodedMaterial);
        write->vertexCursor += RAGE_IMPORT_VERTEX_SIZE;
    }
    for (corner = 0; corner < 6; corner++)
        ImportWrite32(write->indexCursor + corner * 4,
                          write->vertexCount + order[corner]);
    write->indexCursor += 6 * 4;
    write->vertexCount += 4;
    write->indexCount += 6;
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

static RageImportedMeshEntry *ImportBuildMesh(
    const RageRenderMeshInstance *instance) {
    RageImportedTextureKey *materials;
    RageImportedScan scan;
    RageImportedWrite write;
    RageImportedMeshEntry *entry;
    uint32_t meshCount, mesh;
    size_t offsetsSize, verticesSize, indicesSize, total, cursor;
    uint8_t *bytes;
    if (s_entryCount == RAGE_IMPORT_ENTRY_LIMIT) return NULL;
    materials = calloc(RAGE_IMPORT_MATERIAL_LIMIT, sizeof(*materials));
    if (materials == NULL) return NULL;
    memset(&scan, 0, sizeof(scan));
    scan.materials = materials;
    if (!ImportVisit(instance, ImportScanFace, &scan, &meshCount) ||
        scan.faceCount == 0 || scan.faceCount > UINT32_MAX / 6u ||
        !ImportSizeMultiply((size_t)meshCount + 1u, 4u, &offsetsSize) ||
        !ImportSizeMultiply((size_t)scan.faceCount * 4u,
                                RAGE_IMPORT_VERTEX_SIZE, &verticesSize) ||
        !ImportSizeMultiply((size_t)scan.faceCount * 6u, 4u,
                                &indicesSize) ||
        !ImportSizeAdd(RAGE_IMPORT_HEADER_SIZE, offsetsSize, &cursor) ||
        !ImportSizeAdd(cursor, verticesSize, &cursor) ||
        !ImportSizeAdd(cursor, indicesSize, &total)) {
        free(materials);
        return NULL;
    }
    bytes = calloc(1, total);
    if (bytes == NULL) {
        free(materials);
        return NULL;
    }
    memcpy(bytes, "RRMESH1\0", 8);
    ImportWrite32(bytes + 8, 1);
    ImportWrite32(bytes + 12, meshCount);
    ImportWrite32(bytes + 16, (uint32_t)scan.faceCount * 4u);
    ImportWrite32(bytes + 20, (uint32_t)scan.faceCount * 6u);
    entry = &s_entries[s_entryCount];
    memset(entry, 0, sizeof(*entry));
    entry->cached.assetKey = instance->assetKey;
    entry->cached.assetSet = instance->assetSet;
    entry->materials = materials;
    entry->materialCount = scan.materialCount;
    entry->bytes = bytes;
    entry->cached.ownedBytes = bytes;
    memset(&write, 0, sizeof(write));
    write.entry = entry;
    write.vertexCursor = bytes + RAGE_IMPORT_HEADER_SIZE + offsetsSize;
    write.indexCursor = write.vertexCursor + verticesSize;
    if (!ImportVisit(instance, ImportWriteFace, &write, &meshCount) ||
        write.vertexCount != (uint32_t)scan.faceCount * 4u ||
        write.indexCount != (uint32_t)scan.faceCount * 6u) {
        free(entry->bytes);
        free(entry->materials);
        memset(entry, 0, sizeof(*entry));
        return NULL;
    }
    /* Fill mesh offsets with a third bounded pass. */
    for (mesh = 0; mesh <= meshCount; mesh++)
        ImportWrite32(bytes + RAGE_IMPORT_HEADER_SIZE + mesh * 4, 0);
    /* A separate visitor below overwrites these values before the mesh is
     * exposed to the renderer. */
    return entry;
}

/* This first implementation section builds conventional vertices. Mesh range
 * offsets are finalized by ImportFinalizeOffsets, kept separate so the
 * face visitors stay identical for scanning and writing. */
typedef struct RageImportedOffsetWrite {
    uint8_t *offsets;
    uint32_t currentMesh;
    uint32_t indices;
} RageImportedOffsetWrite;

static int ImportOffsetFace(uint32_t mesh, const RageImportedFace *face,
                                void *context) {
    RageImportedOffsetWrite *write = context;
    (void)face;
    while (write->currentMesh < mesh) {
        write->currentMesh++;
        ImportWrite32(write->offsets + write->currentMesh * 4,
                          write->indices);
    }
    write->indices += 6;
    return 1;
}

static int ImportFinalizeOffsets(RageImportedMeshEntry *entry,
                                     const RageRenderMeshInstance *instance,
                                     size_t size) {
    RageImportedOffsetWrite write;
    uint32_t meshCount;
    memset(&write, 0, sizeof(write));
    write.offsets = (uint8_t *)entry->bytes + RAGE_IMPORT_HEADER_SIZE;
    ImportWrite32(write.offsets, 0);
    if (!ImportVisit(instance, ImportOffsetFace, &write, &meshCount))
        return 0;
    while (write.currentMesh < meshCount) {
        write.currentMesh++;
        ImportWrite32(write.offsets + write.currentMesh * 4,
                          write.indices);
    }
    return RuntimeMeshOpen(&entry->cached.mesh, entry->bytes, size);
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
static uint16_t *s_vramSnapshot;
static uint64_t s_vramSnapshotRevision;
static s32 s_vramSnapshotPage;
static int s_haveVramSnapshot;

/*
 * The track's texture pages, assembled rather than waited for.
 *
 * A page is not swapped into video memory in one go: StepTrackTextureSwap
 * moves one row per frame under a time budget, so for several frames after a
 * section change memory holds half of each page. Reading it then is what put
 * a torn texture in the cache and kept it there, always in the same place,
 * because leaving the first tunnel is a section change.
 *
 * Waiting for the swap is not needed, because nothing is ever missing. The
 * swap is an exchange: every row is either in video memory or in the shadow
 * the game keeps in ordinary memory, and g_TrackTextureShadowPage says which.
 * So the page that is wanted can always be put together in full, whatever the
 * cursor has reached. It costs one megabyte held rather than a stall.
 */
enum {
    RAGE_IMPORT_TRACK_ROW_X = 576,   /* g_TrackTextureRect x */
    RAGE_IMPORT_TRACK_ROW_Y = 256,   /* and its y */
    RAGE_IMPORT_TRACK_ROW_W = 448,   /* 0xE0 words, one shadow row */
    RAGE_IMPORT_TRACK_ROWS = 256
};

static uint16_t *s_vramSnapshot;
static uint64_t s_vramSnapshotRevision;
static s32 s_vramSnapshotPage;
static int s_haveVramSnapshot;

/* Put the rows the shadow is holding back where the page expects them. */
static void ImportMergeTrackShadow(uint16_t *vram) {
    s32 row;
    if (g_TrackTextureShadow == NULL) return;
    for (row = 0; row < RAGE_IMPORT_TRACK_ROWS; row++) {
        const uint16_t *shadow;
        uint16_t *target;
        /* A row still holding the wanted page in the shadow has not been
         * exchanged yet, so video memory has the other one. */
        if (g_TrackTextureShadowPage[row] != g_TrackTexturePageWanted) continue;
        shadow = (const uint16_t *)g_TrackTextureShadow[row];
        target = vram + (size_t)(RAGE_IMPORT_TRACK_ROW_Y + row) *
                            RAGE_IMPORT_VRAM_WIDTH + RAGE_IMPORT_TRACK_ROW_X;
        memcpy(target, shadow,
               (size_t)RAGE_IMPORT_TRACK_ROW_W * sizeof(*target));
    }
}

static const uint16_t *ImportVramSnapshot(void) {
    RECT rect = {0, 0, RAGE_IMPORT_VRAM_WIDTH, RAGE_IMPORT_VRAM_HEIGHT};
    uint64_t revision = TrackAssetIdentityRevision();
    if (s_vramSnapshot == NULL) {
        s_vramSnapshot = malloc((size_t)RAGE_IMPORT_VRAM_WIDTH *
                                RAGE_IMPORT_VRAM_HEIGHT *
                                sizeof(*s_vramSnapshot));
        if (s_vramSnapshot == NULL) return NULL;
        s_haveVramSnapshot = 0;
    }
    if (s_haveVramSnapshot && s_vramSnapshotRevision == revision &&
        s_vramSnapshotPage == g_TrackTexturePageWanted)
        return s_vramSnapshot;
    DrawSync(0);
    StoreImage(&rect, (u_long *)s_vramSnapshot);
    DrawSync(0);
    ImportMergeTrackShadow(s_vramSnapshot);
    s_vramSnapshotRevision = revision;
    s_vramSnapshotPage = g_TrackTexturePageWanted;
    s_haveVramSnapshot = 1;
    return s_vramSnapshot;
}

/* Take the next snapshot afresh, for a caller that knows the picture has
 * moved on. */
void NativeAssetImporterInvalidateVram(void) { s_haveVramSnapshot = 0; }

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
        free(s_entries[index].bytes);
        free(s_entries[index].materials);
    }
    memset(s_entries, 0, sizeof(s_entries));
    s_entryCount = 0;
    s_ready = 0;
}

int NativeAssetImporterReady(void) { return s_ready; }

const RageRuntimeCachedMesh *NativeAssetImporterFind(
    const RageRenderMeshInstance *instance) {
    RageImportedMeshEntry *entry;
    size_t offsets, vertices, indices, size;
    if (!s_ready || instance == NULL) return NULL;
    entry = ImportFindEntry(instance->assetKey, instance->assetSet);
    if (entry != NULL) return &entry->cached;
    entry = ImportBuildMesh(instance);
    if (entry == NULL) return NULL;
    offsets = ((size_t)ImportRead32((uint8_t *)entry->bytes + 12) + 1u) * 4u;
    vertices = (size_t)ImportRead32((uint8_t *)entry->bytes + 16) *
               RAGE_IMPORT_VERTEX_SIZE;
    indices = (size_t)ImportRead32((uint8_t *)entry->bytes + 20) * 4u;
    size = RAGE_IMPORT_HEADER_SIZE + offsets + vertices + indices;
    if (!ImportFinalizeOffsets(entry, instance, size)) {
        free(entry->bytes);
        free(entry->materials);
        memset(entry, 0, sizeof(*entry));
        return NULL;
    }
    s_entryCount++;
    fprintf(stderr,
            "rage-port: imported native mesh asset=%u set=%u meshes=%u "
            "materials=%u\n",
            instance->assetKey, (unsigned)instance->assetSet,
            entry->cached.mesh.meshCount, entry->materialCount);
    return &entry->cached;
}

uint32_t NativeAssetImporterMeshCount(void) { return s_entryCount; }

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
    vram = ImportVramSnapshot();
    if (vram == NULL ||
        !ImportDecodeTexture(texture, clut, vram, &pixels,
                                 instance->hasCarPaint ? &paint : NULL)) {
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
                                   ModernAssetImage *image) {
    RageImportedTextureKey texture;
    const uint16_t *vram;
    uint8_t *page = NULL;
    uint8_t *sky;
    uint32_t tile, row, pixel;
    uint32_t opaquePixels = 0;
    (void)assetKey;
    if (!s_ready || image == NULL) return 0;
    memset(image, 0, sizeof(*image));
    memset(&texture, 0, sizeof(texture));
    texture.tpage = 0x18;
    texture.clut = 0x798E;
    vram = ImportVramSnapshot();
    if (vram == NULL ||
        !ImportDecodeTexture(&texture, texture.clut, vram, &page, NULL)) {
        SDL_free(page);
        return 0;
    }
    sky = SDL_calloc(512u * 128u, 4u);
    if (sky == NULL) {
        SDL_free(page);
        return 0;
    }
    for (tile = 0; tile < 8; tile++) {
        uint32_t sourceX = (tile % 4u) * 64u;
        uint32_t sourceY = (tile / 4u) * 128u;
        for (row = 0; row < 128; row++)
            memcpy(sky + (row * 512u + tile * 64u) * 4u,
                   page + ((sourceY + row) * 256u + sourceX) * 4u,
                   64u * 4u);
    }
    SDL_free(page);
    for (pixel = 0; pixel < 512u * 128u; pixel++) {
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
    image->size = 512u * 128u * 4u;
    image->width = 512;
    image->height = 128;
    return 1;
}
