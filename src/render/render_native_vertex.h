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

/* Main vertices occupy the prefix; mirror spans must cover the following
 * mirrorCount vertices in order. Reuse byte-identical main ranges and compact
 * unmatched mirror ranges in place. Returns the physical combined count;
 * logical draw counts and all span state except firstVertex are unchanged.
 * Invalid ranges leave the input unchanged. No cross-frame retention. */
uint32_t RenderShareNativeViewVertices(RageNativeGpuVertex *vertices,
    uint32_t mainCount, const RageNativeDrawSpan *mainSpans, uint32_t mainSpanCount,
    uint32_t mirrorCount, RageNativeDrawSpan *mirrorSpans, uint32_t mirrorSpanCount);

/* Direct compact output; CPU fog encoding remains available for A/B runs. */
uint32_t RenderBuildNativeCompactPassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount);

/* Immutable, material-grouped vehicle geometry shared across instances/views.
 * Initialize with {0}. The caller must release before replacing source meshes.
 * Bounded storage; unsupported geometry or allocation failure uses the regular
 * builder. Transforms and per-instance shading remain evaluated each frame. */
typedef struct RageNativeMeshTemplateCache { void *state; } RageNativeMeshTemplateCache;
/* Borrowed local geometry. The view and its arrays remain at stable addresses
 * until cache release, including when other meshes are acquired. The view
 * address is a cache-lifetime identity suitable for GPU residency lookup.
 * Positions/normals are local and decals are not displaced. Span material,
 * flags, depthDecal and ranges are geometry metadata; instance fields must
 * come from the draw instance, never from these source spans. */
typedef struct RageNativeMeshTemplateView {
    const RageNativeGpuVertex *vertices;
    const RageNativeDrawSpan *spans;
    uint32_t vertexCount, spanCount;
} RageNativeMeshTemplateView;
/* Vehicle asset sets only. NULL means invalid/unsupported input or insufficient
 * cache memory; existing views remain valid. Does not perform view culling. */
const RageNativeMeshTemplateView *RenderNativeMeshTemplateAcquire(
    RageNativeMeshTemplateCache *cache, const RageRuntimeMesh *mesh,
    RageRenderAssetSet assetSet, uint32_t submesh);
void RenderNativeMeshTemplateCacheRelease(RageNativeMeshTemplateCache *cache);
/* Also retain local source/transform metadata for resident GPU draws. Spans
 * are separated at instance/template boundaries. With expandWorldVertices=0,
 * local spans reserve ranges without writing them; use ExpandNativeLocalDraw
 * only when diagnostics or a GPU allocation failure require world vertices. */
uint32_t RenderBuildNativeLocalCompactPassDraws(
    RageNativeMeshTemplateCache *cache,
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog, int expandWorldVertices,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount);
int RenderExpandNativeLocalDraw(const RageNativeDrawSpan *span,
    RageNativeGpuVertex *vertices, uint32_t capacity);
uint32_t RenderBuildNativeCachedCompactPassDraws(
    RageNativeMeshTemplateCache *cache,
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
