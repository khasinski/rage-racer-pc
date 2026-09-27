#include "native_mesh_writer.h"
#include "native_stream.h"
#include "game/model_stream.h"
#include "game/model_bank.h"
#include <stdlib.h>
#include <string.h>

enum { RAGE_IMPORT_MATERIAL_LIMIT = 2048 };

typedef struct RageImportedScan {
    RageImportedTextureKey *materials;
    uint32_t materialCount;
    uint64_t faceCount;
} RageImportedScan;


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




static void ImportWrite32(void *pointer, uint32_t value) {
    uint8_t *p = pointer;
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

int ImportWriteFinish(RageImportedWrite *write, uint32_t meshes) {
    if (write == NULL || write->offsets == NULL || meshes != write->meshLimit ||
        write->currentMesh > meshes || write->vertexCount != write->vertexLimit ||
        write->indexCount != write->indexLimit) return 0;
    while (write->currentMesh < meshes) {
        ++write->currentMesh;
        ImportWrite32(write->offsets + (size_t)write->currentMesh * 4, write->indexCount);
    }
    return 1;
}

int ImportTextureEqual(const RageImportedTextureKey *left,
                                  const RageImportedTextureKey *right) {
    return left->tpage == right->tpage && left->clut == right->clut &&
           left->terrainEnvironmentClut == right->terrainEnvironmentClut &&
           left->hasWindow == right->hasWindow &&
           (!left->hasWindow ||
            (left->windowWidthU == right->windowWidthU &&
             left->windowWidthV == right->windowWidthV &&
             left->windowOffsetU == right->windowOffsetU &&
             left->windowOffsetV == right->windowOffsetV));
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

int ImportWriteFace(uint32_t mesh, const RageImportedFace *face,
                               void *context) {
    RageImportedWrite *write = context;
    uint32_t material = ImportMaterialIndex(write->entry, face);
    uint32_t encodedMaterial = material;
    uint32_t corner;
    static const uint32_t order[] = {0, 2, 1, 1, 2, 3};
    /* The sizing pass is not permission to write beyond its allocation if
     * source topology changes. Check before touching any output byte. */
    if (mesh >= write->meshLimit || mesh < write->currentMesh ||
        write->vertexCount > write->vertexLimit ||
        write->vertexLimit - write->vertexCount < 4 ||
        write->indexCount > write->indexLimit ||
        write->indexLimit - write->indexCount < 6 ||
        (face->textured && material == UINT32_MAX)) return 0;
    while (write->currentMesh < mesh) {
        write->currentMesh++;
        ImportWrite32(write->offsets + (size_t)write->currentMesh * 4,
                      write->indexCount);
    }
    if (face->depthBias != 0 ||
        (write->entry->cached.assetSet == RAGE_RENDER_ASSET_TERRAIN &&
         ((face->flags & 2) != 0 || face->prim < 2 ||
          (face->prim >= 2 && (face->prim & 1) == 0)))) {
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
        if (write->entry->cached.assetSet == RAGE_RENDER_ASSET_TERRAIN &&
            face->prim < 2)
            encodedMaterial |= RAGE_RUNTIME_MATERIAL_FOGGED_NORMAL_ENV;
        else if (write->entry->cached.assetSet == RAGE_RENDER_ASSET_TERRAIN &&
                 (face->prim & 1) == 0)
            encodedMaterial |= RAGE_RUNTIME_MATERIAL_FOGGED;
    }
    if (write->entry->cached.assetSet == RAGE_RENDER_ASSET_COURSE &&
        face->prim == 3 && material != UINT32_MAX)
        encodedMaterial |= RAGE_RUNTIME_MATERIAL_SCROLL_U;
    for (corner = 0; corner < 4; corner++) {
        const SVec *position = &face->vertices[face->vertex[corner]];
        const SVec *normal = face->hasNormals
            ? &face->normals[face->normal[corner]] : NULL;
        RageRuntimeVertex vertex = {
            .position = {(float)position->vx, (float)-position->vy, (float)-position->vz},
            .normal = {normal != NULL ? (float)normal->vx : 0.0f,
                       normal != NULL ? (float)-normal->vy : 1.0f,
                       normal != NULL ? (float)-normal->vz : 0.0f},
            .color = {face->color[0], face->color[1], face->color[2], 255},
            .uv = {face->textured ? ((float)face->uv[corner][0] + 0.5f) / 256.0f : 0.0f,
                   face->textured ? ((float)face->uv[corner][1] + 0.5f) / 256.0f : 0.0f},
            .material = encodedMaterial
        };
        if (!RuntimeVertexEncode(write->vertexCursor, RAGE_RUNTIME_VERTEX_BYTES, &vertex))
            return 0;
        write->vertexCursor += RAGE_RUNTIME_VERTEX_BYTES;
    }
    for (corner = 0; corner < 6; corner++)
        ImportWrite32(write->indexCursor + corner * 4,
                          write->vertexCount + order[corner]);
    write->indexCursor += 6 * 4;
    write->vertexCount += 4;
    write->indexCount += 6;
    return 1;
}

static void ImportReleaseMeshBytes(void *context, const void *bytes) {
    (void)context;
    free((void *)bytes);
}

int ImportBuildMeshEntry(const RenderMeshInstance *instance, RageImportedMeshSource source, void *context, RageImportedMeshEntry *destination) {
    RageImportedTextureKey *materials;
    RageImportedScan scan;
    RageImportedWrite write;
    RageImportedMeshEntry candidate;
    RageImportedMeshEntry *entry = &candidate;
    uint32_t meshCount;
    RageRuntimeMeshLayout layout;
    uint8_t *bytes;
    if (!instance || (unsigned)instance->assetSource >= RENDER_ASSET_SOURCE_COUNT || !source || !destination || destination->materials ||
        destination->carSource || destination->cached.mesh.bytes ||
        destination->cached.ownedBytes || destination->cached.ownedBounds ||
        destination->cached.ownedVertices || destination->cached.ownedIndices) return 0;
    materials = calloc(RAGE_IMPORT_MATERIAL_LIMIT, sizeof(*materials));
    if (materials == NULL) return 0;
    memset(&scan, 0, sizeof(scan));
    scan.materials = materials;
    if (!source(context, ImportScanFace, &scan, &meshCount) ||
        scan.faceCount == 0 || scan.faceCount > UINT32_MAX / 6u ||
        !RuntimeMeshLayout(meshCount, (uint32_t)scan.faceCount * 4u,
                           (uint32_t)scan.faceCount * 6u, &layout)) {
        free(materials);
        return 0;
    }
    bytes = calloc(1, layout.totalSize);
    if (bytes == NULL) {
        free(materials);
        return 0;
    }
    if (!RuntimeMeshEncodeHeader(bytes, layout.totalSize, meshCount,
                                (uint32_t)scan.faceCount * 4u,
                                (uint32_t)scan.faceCount * 6u)) {
        free(bytes);
        free(materials);
        return 0;
    }

    memset(entry, 0, sizeof(*entry));
    entry->source = instance->assetSource;
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
    if (!source(context, ImportWriteFace, &write, &meshCount) ||
        !ImportWriteFinish(&write, meshCount)) {
        free(bytes);
        free(entry->materials);
        memset(entry, 0, sizeof(*entry));
        return 0;
    }
    if (!RuntimeCachedMeshAdopt(&entry->cached, bytes, layout.totalSize,
                                ImportReleaseMeshBytes, NULL)) {
        free(bytes);
        free(entry->materials);
        memset(entry, 0, sizeof(*entry));
        return 0;
    }
    *destination = candidate;
    return 1;
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

int ImportVisitModelBank(const NativeModelBank *bank,
                                    RageImportedFaceVisitor visitor,
                                    void *context, uint32_t *meshCount) {
    uint32_t mesh;
    if (bank == NULL || visitor == NULL || meshCount == NULL ||
        bank->modelCount <= 0 || bank->modelCount > GAME_MODEL_PER_BANK_LIMIT ||
        bank->table == NULL) return 0;
    *meshCount = (uint32_t)bank->modelCount;
    for (mesh = 0; mesh < *meshCount; mesh++) {
        if (bank->models[mesh] == NULL ||
            !ImportVisitModelStream(mesh, bank->models[mesh], bank->table,
                                        bank->normals, visitor, context))
            return 0;
    }
    return 1;
}

static int VisitBank(void *context, RageImportedFaceVisitor visitor,
                     void *output, uint32_t *meshCount) {
    return ImportVisitModelBank(context, visitor, output, meshCount);
}

int ImportBuildBankMesh(const RenderMeshInstance *instance, const NativeModelBank *bank,
                        RageImportedMeshEntry *destination) {
    return ImportBuildMeshEntry(instance, VisitBank, (void *)bank, destination);
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

int ImportVisitCourseModels(const NativeCourseModel *models, s32 count,
                            RageImportedFaceVisitor visitor, void *context,
                            uint32_t *meshCount) {
    if (!models || !visitor || !meshCount || count <= 0 ||
        count > GAME_COURSE_MODEL_LIMIT) return 0;
    *meshCount = (uint32_t)count;
    for (uint32_t mesh = 0; mesh < *meshCount; ++mesh) {
        if (!models[mesh].geometry || !models[mesh].model ||
            !ImportVisitCourseStream(mesh, models[mesh].model, models[mesh].geometry,
                                    visitor, context)) return 0;
    }
    return 1;
}

static int VisitCourse(void *context, RageImportedFaceVisitor visitor,
                       void *output, uint32_t *meshCount) {
    const CourseBank *bank = context;
    if (!bank) return 0;
    return ImportVisitCourseModels(bank->models, bank->modelCount, visitor, output, meshCount);
}

int ImportBuildCourseMesh(const RenderMeshInstance *instance, const CourseBank *bank,
                          RageImportedMeshEntry *destination) {
    return ImportBuildMeshEntry(instance, VisitCourse, (void *)bank, destination);
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
            /* Modes 2..5 encode their palette directly. Only 0/1 follow
             * envMode4, as in the retail terrain dispatch. Keep that choice
             * in the material key even when the base atlas and CLUT match. */
            value.texture.terrainEnvironmentClut = prim < 2;
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

int ImportVisitTerrainCells(const void *const *cells, s32 count, const SVec *vertices,
                            RageImportedFaceVisitor visitor, void *context,
                            uint32_t *meshCount) {
    if (!cells || !vertices || !visitor || !meshCount || count <= 0 ||
        count > GAME_TERRAIN_CELL_LIMIT) return 0;
    *meshCount = (uint32_t)count;
    for (uint32_t mesh = 0; mesh < *meshCount; ++mesh) {
        if (!cells[mesh] ||
            !ImportVisitTerrainStream(mesh, cells[mesh], vertices, visitor, context)) return 0;
    }
    return 1;
}

static int VisitTerrain(void *context, RageImportedFaceVisitor visitor,
                        void *output, uint32_t *meshCount) {
    const TerrainBank *bank = context;
    if (!bank) return 0;
    return ImportVisitTerrainCells(bank->cells, bank->cellCount, bank->vertices,
                                   visitor, output, meshCount);
}

int ImportBuildTerrainMesh(const RenderMeshInstance *instance, const TerrainBank *bank,
                           RageImportedMeshEntry *destination) {
    return ImportBuildMeshEntry(instance, VisitTerrain, (void *)bank, destination);
}
