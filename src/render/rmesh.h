#ifndef RAGE_RMESH_H
#define RAGE_RMESH_H

#include <stddef.h>
#include <stdint.h>

/* Reader for tools/assetbrowser/rmesh.py output.  It is intentionally a
 * bounded view into caller-owned bytes: asset I/O and GPU upload stay outside
 * the renderer-neutral asset contract. */

typedef struct RageRuntimeMesh {
    const uint8_t *bytes;
    size_t size;
    uint32_t meshCount;
    uint32_t vertexCount;
    uint32_t indexCount;
    size_t offsetsOffset;
    size_t verticesOffset;
    size_t indicesOffset;
} RageRuntimeMesh;

typedef struct RageRuntimeVertex {
    float position[3];
    float normal[3];
    uint8_t color[4];
    float uv[2];
    uint32_t material;
} RageRuntimeVertex;

int RageRuntimeMeshOpen(RageRuntimeMesh *mesh, const void *bytes, size_t size);
int RageRuntimeMeshRange(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                         uint32_t *firstIndex, uint32_t *indexCount);
int RageRuntimeMeshVertex(const RageRuntimeMesh *mesh, uint32_t vertexIndex,
                          RageRuntimeVertex *out);
int RageRuntimeMeshIndex(const RageRuntimeMesh *mesh, uint32_t indexIndex,
                         uint32_t *out);
/* Conservative local-space sphere of a submesh, calculated from indexed
 * vertices. It is renderer-neutral and lets native backends cull before
 * expanding a mesh into draw vertices. */
int RageRuntimeMeshBounds(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                          float center[3], float *radius);

#endif

