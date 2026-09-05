#ifndef RAGE_NATIVE_MESH_WRITER_H
#define RAGE_NATIVE_MESH_WRITER_H

#include "common.h"
#include "game/vector.h"
#include "render/rmesh_cache.h"

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

typedef struct RageImportedWrite {
    RageImportedMeshEntry *entry;
    uint8_t *offsets;
    uint8_t *vertexCursor;
    uint8_t *indexCursor;
    uint32_t currentMesh;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t meshLimit;
    uint32_t vertexLimit;
    uint32_t indexLimit;
} RageImportedWrite;

/* Private synchronous importer writer. Source arrays are borrowed during the
 * call; output storage/capacities belong to the sizing pass. */
int ImportTextureEqual(const RageImportedTextureKey *left,
                       const RageImportedTextureKey *right);
int ImportWriteFace(uint32_t mesh, const RageImportedFace *face, void *context);
/* Verify the second pass matches its sizing limits before filling trailing
 * empty submesh endpoints. Rejection leaves writer and output unchanged. */
int ImportWriteFinish(RageImportedWrite *write, uint32_t meshes);

#endif
