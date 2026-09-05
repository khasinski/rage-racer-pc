#ifndef RAGE_RMESH_H
#define RAGE_RMESH_H

#include <stddef.h>
#include <stdint.h>

/* Reader for tools/assetbrowser/rmesh.py output.  It is intentionally a
 * bounded view into caller-owned bytes: asset I/O and GPU upload stay outside
 * the renderer-neutral asset contract. */

typedef struct RageRuntimeMeshBounds {
    float center[3], radius;
    int valid;
} RageRuntimeMeshBounds;

typedef struct RageRuntimeMesh {
    const uint8_t *bytes;
    size_t size;
    uint32_t meshCount;
    uint32_t vertexCount;
    uint32_t indexCount;
    size_t offsetsOffset;
    size_t verticesOffset;
    size_t indicesOffset;
    /* Optional caller-owned cache; bytes must remain immutable while attached.
     * RuntimeMeshOpen clears this pointer, including on failure. */
    const RageRuntimeMeshBounds *bounds;
} RageRuntimeMesh;

typedef struct RageRuntimeVertex {
    float position[3];
    float normal[3];
    uint8_t color[4];
    float uv[2];
    uint32_t material;
} RageRuntimeVertex;

enum {
    /* Stored in the otherwise-small material index by rmesh.py. The draw
     * builder removes it before material lookup and applies the instance's
     * semantic U offset to that vertex. UINT32_MAX remains untextured. */
    RAGE_RUNTIME_MATERIAL_SCROLL_U = 1u << 31,
    RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY = 1u << 30,
    RAGE_RUNTIME_MATERIAL_METADATA = 1u << 29,
    RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT = 1u << 28,
    RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT = 16,
    RAGE_RUNTIME_MATERIAL_INDEX_MASK = 0xFFFFu,
};

int RuntimeMeshOpen(RageRuntimeMesh *mesh, const void *bytes, size_t size);
int RuntimeMeshRange(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                         uint32_t *firstIndex, uint32_t *indexCount);
int RuntimeMeshVertex(const RageRuntimeMesh *mesh, uint32_t vertexIndex,
                          RageRuntimeVertex *out);
int RuntimeMeshIndex(const RageRuntimeMesh *mesh, uint32_t indexIndex,
                         uint32_t *out);
/* Conservative local-space sphere of a submesh, calculated from indexed
 * vertices. It is renderer-neutral and lets native backends cull before
 * expanding a mesh into draw vertices. */
int RuntimeMeshBounds(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                          float center[3], float *radius);
int RuntimeMeshPrepareBounds(RageRuntimeMesh *mesh,
                            RageRuntimeMeshBounds *storage, size_t capacity);

#endif
