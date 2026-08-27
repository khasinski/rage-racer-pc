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
    /* Dynamic vehicle geometry casts onto the world but does not sample the
     * same map back onto itself. Its low-poly surface otherwise exposes
     * shadow-map acne along every source triangle. */
    float shadowReception;
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
    uint8_t hasCarPaint;
    uint8_t carPaintColor1;
    uint8_t carPaintColor2;
    /* Semantic multipart slot from RageRenderMeshInstance. Vehicle body is
     * component 0; wheels retain their own slots so materials do not mistake
     * rubber and rims for clear-coated bodywork. */
    uint8_t component;
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
/* Builds only one semantic pass. Native mirrors deliberately render the main
 * scene again from another camera instead of consuming PS1 mirror instances. */
uint32_t RageRenderBuildNativePassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount);

#endif
