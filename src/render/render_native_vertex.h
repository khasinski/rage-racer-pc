#ifndef RAGE_RENDER_NATIVE_VERTEX_H
#define RAGE_RENDER_NATIVE_VERTEX_H

#include <string.h>
#include "render_mesh_build.h"

/* GPU geometry payload only. Lighting and shadow reception are draw-constant
 * instance uniforms; the expanded CPU vertex remains the diagnostic oracle. */
typedef struct RageNativeGpuVertex {
    float position[3];
    float uv[2];
    uint8_t color[4];
    float normal[3];
    float fog[4];
    float depthBias;
} RageNativeGpuVertex;

_Static_assert(sizeof(RageNativeGpuVertex) == 56, "native GPU vertex ABI");

/* Direct compact output; CPU fog encoding remains available for A/B runs. */
uint32_t RenderBuildNativeCompactPassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount);

static inline RageNativeGpuVertex RenderPackNativeGpuVertex(
    const RageNativeDrawVertex *source) {
    RageNativeGpuVertex vertex;
    memcpy(vertex.position, source->position, sizeof(vertex.position));
    memcpy(vertex.uv, source->uv, sizeof(vertex.uv));
    memcpy(vertex.color, source->color, sizeof(vertex.color));
    memcpy(vertex.normal, source->normal, sizeof(vertex.normal));
    memcpy(vertex.fog, source->fog, sizeof(vertex.fog));
    vertex.depthBias = source->depthBias;
    return vertex;
}

#endif
