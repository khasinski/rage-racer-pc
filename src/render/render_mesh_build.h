#ifndef RAGE_RENDER_MESH_BUILD_H
#define RAGE_RENDER_MESH_BUILD_H

#include "render_projection.h"
#include "rmesh.h"

typedef struct RageNativeDrawVertex {
    /* Projection belongs to the GPU, retaining hardware near-plane clipping. */
    float position[3];
    float uv[2];
    uint8_t color[4];
    float normal[3];
} RageNativeDrawVertex;

typedef struct RageNativeDrawSpan {
    uint32_t firstVertex;
    uint32_t vertexCount;
    uint32_t assetKey;
    RageRenderAssetSet assetSet;
    uint32_t material;
    /* Material variants (for example each car's paint palette) are scoped to
     * an entity, not to the shared immutable mesh asset. */
    uint32_t entity;
    RageRenderPass pass;
} RageNativeDrawSpan;

typedef const RageRuntimeMesh *(*RageRenderMeshLookup)(
    void *context, const RageRenderMeshInstance *instance);

/* Expands imported indexed meshes into GPU-ready triangles. Fully outside
 * triangles are skipped; near-plane clipping itself belongs to the GPU. */
uint32_t RageRenderBuildNativeDraws(const RageRenderWorld *world, float aspect,
                                    RageRenderMeshLookup lookup, void *context,
                                    RageNativeDrawVertex *vertices,
                                    uint32_t vertexCapacity,
                                    RageNativeDrawSpan *spans,
                                    uint32_t spanCapacity,
                                    uint32_t *spanCount);

#endif

