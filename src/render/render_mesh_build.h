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
    /* RGB environment colour plus the per-vertex perspective fog weight. */
    float fog[4];
    float lighting;
    float environmentLight[3];
    /* Signed authored ordering offset, applied as a tiny clip-space decal
     * bias by the GPU.  It preserves coplanar road markings without moving
     * geometry in world space. */
    float depthBias;
} RageNativeDrawVertex;

typedef struct RageNativeDrawSpan {
    uint32_t firstVertex;
    uint32_t vertexCount;
    uint32_t assetKey;
    RageRenderAssetSet assetSet;
    uint32_t mesh;
    uint32_t sourceEntity;
    uint32_t instanceFlags;
    uint32_t material;
    uint32_t materialFlags;
    /* Authored negative terrain OT bias denotes a coplanar overlay such as a
     * road marking. The GPU uses a slope-scaled raster bias for these spans;
     * the per-vertex bias alone is not stable at grazing camera angles. */
    uint8_t depthDecal;
    uint8_t materialVariant;
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
