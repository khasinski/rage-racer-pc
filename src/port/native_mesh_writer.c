#include "native_mesh_writer.h"

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
