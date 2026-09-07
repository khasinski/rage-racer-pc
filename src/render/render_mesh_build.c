#include "render_mesh_build.h"
#include "render_native_vertex.h"
#include "render_instance_transform.h"
#include "render_triangle_geometry.h"
#include "authored_car_surface.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static float Radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

typedef RageRenderInstanceTransform RageTransformBasis;

static RageRenderVec3 TransformPosition(const RageTransformBasis *basis,
                                             const RageRuntimeVertex *vertex) {
    return RenderTransformInstancePoint(basis,
        (RageRenderVec3){vertex->position[0], vertex->position[1], vertex->position[2]});
}

static float SnapTerrainCellBoundary(float value) {
    const float cellSize = 2048.0f;
    float boundary = roundf(value / cellSize) * cellSize;
    /* Adjacent PS1 terrain cells occasionally disagree by one source GTE
     * unit (0.25 native world units). The original low-resolution rasterizer
     * hid that quantization gap; weld only vertices already on a cell edge. */
    return fabsf(value - boundary) <= 0.5f ? boundary : value;
}

static RageRenderVec3 TransformNormal(const RageTransformBasis *basis,
                                          const RageRuntimeVertex *vertex) {
    RageRenderVec3 out = {vertex->normal[0], vertex->normal[1], vertex->normal[2]};
    return RenderRotateInstanceVector(basis, out);
}

static RageRenderVec3 TransformPoint(const RageTransformBasis *basis,
                                         const float position[3]) {
    return RenderTransformInstancePoint(basis,
        (RageRenderVec3){position[0], position[1], position[2]});
}

static float Vec3Length(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

/* Triangle shape is independent of the camera. Keep the unnormalised normal
 * and its length so flat shading and displacement share exactly the same
 * geometry calculation, without baking view-facing orientation into it. */
static void PrepareTriangleGeometry(const RageNativeDrawVertex triangle[3],
                                    RageTriangleGeometry *geometry) {
    if (geometry->prepared) return;
    RageRenderVec3 positions[3];
    for (unsigned corner = 0; corner < 3; ++corner) {
        positions[corner] = (RageRenderVec3){triangle[corner].position[0],
            triangle[corner].position[1], triangle[corner].position[2]};
    }
    *geometry = RenderTriangleGeometry(positions);
}

static void ApplyFlatTriangleNormal(RageNativeDrawVertex triangle[3],
                                    RageTriangleGeometry *geometry) {
    PrepareTriangleGeometry(triangle, geometry);
    float nx = geometry->nx, ny = geometry->ny, nz = geometry->nz;
    float length = geometry->length;
    uint32_t corner;
    if (length <= 0.000001f) return;
    nx /= length;
    ny /= length;
    nz /= length;
    for (corner = 0; corner < 3; corner++) {
        triangle[corner].normal[0] = nx;
        triangle[corner].normal[1] = ny;
        triangle[corner].normal[2] = nz;
    }
}

/* Road paint is ordinary native geometry: a long, narrow strip following the
 * road surface. Identify that semantic shape without consulting PS1 primitive
 * modes or ordering-table hints. */
static int TriangleIsRoadDecal(const RageNativeDrawVertex triangle[3],
                               RageTriangleGeometry *geometry) {
    RageRenderVec3 positions[3];
    for (unsigned corner = 0; corner < 3; ++corner) {
        positions[corner] = (RageRenderVec3){triangle[corner].position[0],
            triangle[corner].position[1], triangle[corner].position[2]};
    }
    return RenderTriangleIsRoadDecal(positions, geometry);
}

static void LiftRoadDecal(RageNativeDrawVertex triangle[3],
                          RageTriangleGeometry *geometry) {
    PrepareTriangleGeometry(triangle, geometry);
    float nx = geometry->nx, ny = geometry->ny, nz = geometry->nz;
    float length = geometry->length;
    int corner;
    if (length <= 0.0f) return;
    if (ny < 0.0f) length = -length;
    nx /= length; ny /= length; nz /= length;
    for (corner = 0; corner < 3; corner++) {
        triangle[corner].position[0] += nx * 2.0f;
        triangle[corner].position[1] += ny * 2.0f;
        triangle[corner].position[2] += nz * 2.0f;
    }
}

static void LiftOverlayTowardCamera(
    RageNativeDrawVertex triangle[3], RageRenderVec3 camera,
    RageTriangleGeometry *geometry) {
    PrepareTriangleGeometry(triangle, geometry);
    float nx = geometry->nx, ny = geometry->ny, nz = geometry->nz;
    float cx = (triangle[0].position[0] + triangle[1].position[0] +
                triangle[2].position[0]) / 3.0f;
    float cy = (triangle[0].position[1] + triangle[1].position[1] +
                triangle[2].position[1]) / 3.0f;
    float cz = (triangle[0].position[2] + triangle[1].position[2] +
                triangle[2].position[2]) / 3.0f;
    float length = geometry->length;
    float facing;
    int corner;
    if (length <= 0.0f) return;
    nx /= length; ny /= length; nz /= length;
    facing = nx * (camera.x - cx) + ny * (camera.y - cy) +
             nz * (camera.z - cz);
    if (facing < 0.0f) {
        nx = -nx; ny = -ny; nz = -nz;
    }
    for (corner = 0; corner < 3; corner++) {
        triangle[corner].position[0] += nx * 2.0f;
        triangle[corner].position[1] += ny * 2.0f;
        triangle[corner].position[2] += nz * 2.0f;
    }
}

static void LiftCarDecal(RageNativeDrawVertex triangle[3]) {
    int corner, axis;
    /* A hood marking stays outside the painted panel even when the camera
     * looks across it at a grazing angle. Its authored normals point out. */
    for (corner = 0; corner < 3; ++corner) {
        float *n = triangle[corner].normal;
        float length = Vec3Length(n[0], n[1], n[2]);
        if (length <= 0.0f) continue;
        for (axis = 0; axis < 3; ++axis)
            triangle[corner].position[axis] += n[axis] * (2.0f / length);
    }
}

static int TriangleIsBackFacing(const RageRenderWorld *world,
                                    const RageRenderViewTransform *viewTransform,
                                    const RageNativeDrawVertex triangle[3]) {
    RageRenderVec3 view[3];
    float screenX[3], screenY[3];
    int corner;
    for (corner = 0; corner < 3; corner++) {
        RageRenderVec3 position = {triangle[corner].position[0],
                                   triangle[corner].position[1],
                                   triangle[corner].position[2]};
        float depth;
        RenderWorldToViewPrepared(viewTransform, &position, &view[corner]);
        depth = -view[corner].z;
        /* Let homogeneous clipping handle triangles crossing the camera.
         * Their projected winding is undefined until after the clip. */
        if (depth <= world->camera.nearPlane) return 0;
        screenX[corner] = view[corner].x / depth;
        screenY[corner] = view[corner].y / depth;
    }
    /* Runtime indices use clockwise front faces in the renderer's Y-up
     * projection. This is the same side selected by the game's NCLIP path. */
    return (screenX[1] - screenX[0]) * (screenY[2] - screenY[0]) -
           (screenY[1] - screenY[0]) * (screenX[2] - screenX[0]) >= 0.0f;
}

static uint32_t ClipViewTriangleNear(
    const RageRenderVec3 input[3], RageRenderVec3 output[4], float nearPlane) {
    RageRenderVec3 previous = input[2];
    int previousInside = -previous.z >= nearPlane;
    uint32_t inputIndex, count = 0;
    for (inputIndex = 0; inputIndex < 3; inputIndex++) {
        RageRenderVec3 current = input[inputIndex];
        int currentInside = -current.z >= nearPlane;
        if (currentInside != previousInside) {
            float boundaryZ = -nearPlane;
            float t = (boundaryZ - previous.z) / (current.z - previous.z);
            RageRenderVec3 clipped = {
                previous.x + (current.x - previous.x) * t,
                previous.y + (current.y - previous.y) * t,
                boundaryZ,
            };
            output[count++] = clipped;
        }
        if (currentInside) output[count++] = current;
        previous = current;
        previousInside = currentInside;
    }
    return count;
}

static int TerrainTriangleFacesCamera(
    const RageRenderWorld *world, const RageRenderViewTransform *viewTransform,
    const RageRenderVec3 triangle[3]) {
    RageRenderVec3 input[3], clipped[4];
    uint32_t corner, count, piece;
    for (corner = 0; corner < 3; corner++) {
        RenderWorldToViewPrepared(viewTransform, &triangle[corner], &input[corner]);
    }
    count = ClipViewTriangleNear(
        input, clipped, world->camera.nearPlane);
    for (piece = 1; piece + 1 < count; piece++) {
        RageRenderVec3 view[3] = {clipped[0], clipped[piece],
                                  clipped[piece + 1]};
        float screenX[3], screenY[3], area;
        for (corner = 0; corner < 3; corner++) {
            float depth = -view[corner].z;
            screenX[corner] = view[corner].x / depth;
            screenY[corner] = view[corner].y / depth;
        }
        area = (screenX[1] - screenX[0]) *
                   (screenY[2] - screenY[0]) -
               (screenY[1] - screenY[0]) *
                   (screenX[2] - screenX[0]);
        if (area > 0.0f) return 1;
    }
    return 0;
}

static int TerrainQuadIsHidden(
    const RageRenderWorld *world, const RageTransformBasis *basis,
    const RageRenderViewTransform *viewTransform,
    const RageRuntimeMesh *mesh, uint32_t first,
    RageRenderVec3 positions[6], int *positionsValid) {
    *positionsValid = 0;
    /* Visibility has no UV, normal, fog or lighting dependency. Keep its
     * temporary geometry independent of the expanded shading payload. */
    RageRenderVec3 triangles[2][3];
    uint32_t indices[6];
    static const uint8_t uniqueCorners[4] = {0, 1, 2, 5};
    uint32_t corner;
    for (corner = 0; corner < 6; corner++)
        if (!RuntimeMeshIndex(mesh, first + corner, &indices[corner]))
            return 0;
    /* rmesh terrain faces are authored quads expanded as ABC/CBD. Do not
     * infer quad culling for an independent triangle pair from a mod. */
    if (indices[3] != indices[2] || indices[4] != indices[1]) return 0;
    for (corner = 0; corner < 4; corner++) {
        RageRuntimeVertex source;
        RageRenderVec3 position;
        uint32_t target = uniqueCorners[corner];
        if (!RuntimeMeshVertex(mesh, indices[target], &source))
            return 0;
        position = TransformPosition(basis, &source);
        position.x = SnapTerrainCellBoundary(position.x);
        position.z = SnapTerrainCellBoundary(position.z);
        if (corner < 3) triangles[0][corner] = position;
        else triangles[1][2] = position;
    }
    triangles[1][0] = triangles[0][2];
    triangles[1][1] = triangles[0][1];
    /* Reuse snapped world positions for shading this same authored quad.
     * Invalid/non-quad pairs never publish positions. The caller refreshes
     * this storage at every six-index boundary and for every instance. */
    for (corner = 0; corner < 6; ++corner)
        positions[corner] = triangles[corner / 3][corner % 3];
    *positionsValid = 1;
    /* A terrain face is one authored quad. Reject it only when neither half
     * faces the camera. Slightly twisted quads otherwise lose valid road
     * geometry when culled per triangle, while drawing both sides exposes
     * hidden wall backs as large dark polygons. */
    /* Terrain source quads use the opposite winding from course objects
     * after their independent source-to-scene import conversion. Evaluate
     * that winding after clipping so a hidden wall crossing the camera does
     * not expand into a screen-sized polygon. */
    return !TerrainTriangleFacesCamera(world, viewTransform, triangles[0]) &&
           !TerrainTriangleFacesCamera(world, viewTransform, triangles[1]);
}

/* Camera-only values; never retain them in an asset or instance cache. */
typedef struct RageInstanceFrustum {
    float tanX, tanY, horizontalScale, verticalScale;
} RageInstanceFrustum;

static RageInstanceFrustum PrepareInstanceFrustum(
    const RageRenderWorld *world, float aspect) {
    RageInstanceFrustum result;
    float tanY = tanf(Radians(world->camera.verticalFovDegrees) * 0.5f);
    float tanX = tanY * aspect;
    /* Preserve the guard band and operation order of the per-instance test. */
    result.tanX = tanX * 1.08f;
    result.tanY = tanY * 1.08f;
    result.horizontalScale = sqrtf(1.0f + result.tanX * result.tanX);
    result.verticalScale = sqrtf(1.0f + result.tanY * result.tanY);
    return result;
}

static int InstanceOutsideFrustum(const RageRenderWorld *world,
                                      const RageRenderViewTransform *viewTransform,
                                      const RageInstanceFrustum *frustum,
                                      const RageTransformBasis *basis,
                                      const RageRuntimeMesh *mesh,
                                      uint32_t meshIndex) {
    float center[3], radius, maxScale, depth;
    float horizontalRadius, verticalRadius;
    RageRenderVec3 worldCenter, view;
    if (!RuntimeMeshBounds(mesh, meshIndex, center, &radius)) return 0;
    worldCenter = TransformPoint(basis, center);
    RenderWorldToViewPrepared(viewTransform, &worldCenter, &view);
    depth = -view.z;
    maxScale = fmaxf(fabsf(basis->scale.x),
                     fmaxf(fabsf(basis->scale.y), fabsf(basis->scale.z)));
    radius *= maxScale;
    if (depth + radius < world->camera.nearPlane ||
        depth - radius > world->camera.farPlane) return 1;
    /* Keep a small guard band around the visible frustum. In a low cockpit
     * camera the road can cross the side plane between logic ticks on a
     * sharp bend; exact-edge culling otherwise exposes a one-cell notch for
     * a frame before the interpolated camera catches up. */
    /* Test the sphere against the actual side planes. Comparing its
     * axis-aligned radius with the frustum width at the sphere centre is not
     * conservative: a large nearby terrain cell can cross a side plane even
     * when its centre is well outside it. */
    horizontalRadius = radius * frustum->horizontalScale;
    verticalRadius = radius * frustum->verticalScale;
    return fabsf(view.x) > depth * frustum->tanX + horizontalRadius ||
           fabsf(view.y) > depth * frustum->tanY + verticalRadius;
}

/* These values belong to an instance, not its immutable source vertices or
 * camera. Resolve compatibility defaults once, before visiting the mesh. */
static RageNativeInstanceState PrepareInstanceState(
    const RageRenderMeshInstance *instance) {
    RageNativeInstanceState state = {0};
    if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_LIGHTING) != 0) {
        state.lighting = instance->lightInfluence;
        if (state.lighting <= 0.0f) state.lighting = 1.0f;
        if (state.lighting > 1.0f) state.lighting = 1.0f;
    }
    state.environmentLight[0] = instance->environmentLight.x;
    state.environmentLight[1] = instance->environmentLight.y;
    state.environmentLight[2] = instance->environmentLight.z;
    if (state.environmentLight[0] == 0.0f &&
        state.environmentLight[1] == 0.0f &&
        state.environmentLight[2] == 0.0f) {
        /* Zero-initialized legacy callers request neutral light. */
        state.environmentLight[0] = 1.0f;
        state.environmentLight[1] = 1.0f;
        state.environmentLight[2] = 1.0f;
    }
    state.shadowReception =
        instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
        instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1
        ? 0.0f : 1.0f;
    return state;
}

static int BuildVertex(const RageTransformBasis *basis,
                           const RageRenderViewTransform *viewTransform,
                           const RageRenderWorld *world, int fogged, int gpuFog, int gpuUV,
                           const RageRenderMeshInstance *instance,
                           const RageNativeInstanceState *instanceState,
                           const RageRuntimeMesh *mesh, uint32_t index,
                           const RageRenderVec3 *preparedPosition,
                           float aspect, int localSource, RageNativeDrawVertex *out,
                           uint32_t *material, uint32_t *materialFlags,
                           uint8_t *depthDecal) {
    RageRuntimeVertex source;
    RageRenderVec3 normal;
    RageRenderVec3 worldPosition;
    if (!RuntimeMeshVertex(mesh, index, &source)) return 0;
    *materialFlags = source.material &
        (RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY |
         RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT);
    worldPosition = localSource ? (RageRenderVec3){source.position[0], source.position[1], source.position[2]} :
        preparedPosition != NULL ? *preparedPosition
                                             : TransformPosition(basis, &source);
    if (preparedPosition == NULL && instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
        worldPosition.x = SnapTerrainCellBoundary(worldPosition.x);
        worldPosition.z = SnapTerrainCellBoundary(worldPosition.z);
    }
    (void)aspect;
    out->position[0] = worldPosition.x; out->position[1] = worldPosition.y;
    out->position[2] = worldPosition.z;
    out->uv[0] = source.uv[0]; out->uv[1] = source.uv[1];
    if (source.material != UINT32_MAX &&
        (source.material & RAGE_RUNTIME_MATERIAL_SCROLL_U) != 0) {
        if (gpuUV) *materialFlags |= RAGE_RUNTIME_MATERIAL_SCROLL_U;
        else out->uv[0] += (float)instance->textureScrollU * (1.0f / 256.0f);
        source.material &= ~RAGE_RUNTIME_MATERIAL_SCROLL_U;
    }
    memcpy(out->color, source.color, sizeof(out->color));
    normal = localSource ? (RageRenderVec3){source.normal[0], source.normal[1], source.normal[2]} :
        TransformNormal(basis, &source);
    out->normal[0] = normal.x;
    out->normal[1] = normal.y;
    out->normal[2] = normal.z;
    if (gpuFog) {
        out->fog[0] = worldPosition.x;
        out->fog[1] = worldPosition.y;
        out->fog[2] = worldPosition.z;
        out->fog[3] = fogged ? 1.0f : 0.0f;
    } else {
        out->fog[0] = world->camera.fogColor.x;
        out->fog[1] = world->camera.fogColor.y;
        out->fog[2] = world->camera.fogColor.z;
        out->fog[3] = fogged
            ? RenderFogFactorPrepared(viewTransform, &worldPosition) : 0.0f;
    }
    out->lighting = instanceState->lighting;
    memcpy(out->environmentLight, instanceState->environmentLight,
           sizeof(out->environmentLight));
    out->depthBias = 0.0f;
    out->shadowReception = instanceState->shadowReception;
    *depthDecal =
        (instance->flags & RAGE_RENDER_INSTANCE_DEPTH_DECAL) != 0;
    if ((source.material & RAGE_RUNTIME_MATERIAL_METADATA) != 0) {
        /* Terrain's packed signed OT offset distinguishes coplanar course
         * layers (for example a chevron barrier from its backing strip).
         * Preserve it as the native renderer's tiny depth separation.  It
         * is intentionally not an instance-wide nudge: each face retains
         * the exact ordering the retail emitter authored. */
        if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN)
            out->depthBias = (float)(int8_t)(source.material >>
                RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT);
        source.material &= RAGE_RUNTIME_MATERIAL_INDEX_MASK;
        if (source.material == RAGE_RUNTIME_MATERIAL_INDEX_MASK)
            source.material = UINT32_MAX;
    }
    *material = source.material;
    if ((instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
         instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) &&
        source.material != UINT32_MAX &&
        source.material / RAGE_CAR_SURFACE_RUNTIME_STRIDE == RAGE_CAR_SURFACE_DECAL)
        *depthDecal = 1;
    return 1;
}

typedef struct RagePreparedVertexCacheEntry {
    uint32_t instanceEpoch, sourceIndex;
    RageNativeDrawVertex vertex;
    uint32_t material, materialFlags;
    uint8_t depthDecal;
} RagePreparedVertexCacheEntry;

typedef struct RageNativeMeshTemplate {
    struct RageNativeMeshTemplate *next;
    const RageRuntimeMesh *source;
    uint32_t mesh;
    RageRenderAssetSet assetSet;
    RageNativeGpuVertex *vertices;
    RageNativeDrawSpan *spans;
    uint32_t vertexCount, spanCount;
    RageNativeMeshTemplateView view;
} RageNativeMeshTemplate;

typedef struct RageNativeMeshTemplateState {
    RageNativeMeshTemplate *first;
    size_t bytes;
} RageNativeMeshTemplateState;

static const RageRuntimeMesh *TemplateMeshLookup(void *context,
    const RageRenderMeshInstance *instance) {
    (void)instance;
    return context;
}

static int SpanMatches(const RageNativeDrawSpan *span,
    const RageRenderMeshInstance *instance, const RageNativeInstanceState *state,
    uint32_t material, uint32_t flags, uint8_t decal, uint8_t variant) {
    return span->material == material && span->materialFlags == flags &&
        span->depthDecal == decal && span->assetKey == instance->assetKey &&
        span->assetSet == instance->assetSet && span->mesh == instance->mesh &&
        span->sourceEntity == instance->entity && span->instanceFlags == instance->flags &&
        !memcmp(&span->instanceState, state, sizeof(*state)) &&
        span->materialVariant == variant && span->hasCarPaint == instance->hasCarPaint &&
        span->carPaintColor1 == instance->carPaintColor1 &&
        span->carPaintColor2 == instance->carPaintColor2 && span->component == instance->component &&
        span->entity == (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? instance->entity : 0) &&
        span->pass == instance->pass;
}

static void TransformTemplateVertices(const RageNativeGpuVertex *source, uint32_t count,
    const RageTransformBasis *basis, uint32_t flags, int decal, RageNativeGpuVertex *vertices) {
    for (uint32_t v = 0; v < count; ++v) {
        const RageNativeGpuVertex *input = &source[v];
        RageNativeGpuVertex *output = &vertices[v];
        RageRenderVec3 position = RenderTransformInstancePoint(basis,
            (RageRenderVec3){input->position[0], input->position[1], input->position[2]});
        RageRenderVec3 normal = RenderRotateInstanceVector(basis,
            (RageRenderVec3){input->normal[0], input->normal[1], input->normal[2]});
        *output = *input;
        output->fog[0] = position.x; output->fog[1] = position.y; output->fog[2] = position.z;
        output->fog[3] = (flags & RAGE_RENDER_INSTANCE_ENABLE_FOG) ? 1.0f : 0.0f;
        output->normal[0] = normal.x; output->normal[1] = normal.y; output->normal[2] = normal.z;
        if (decal) {
            float length = Vec3Length(normal.x, normal.y, normal.z);
            if (length > 0) {
                position.x += normal.x * (2.0f / length);
                position.y += normal.y * (2.0f / length);
                position.z += normal.z * (2.0f / length);
            }
        }
        output->position[0] = position.x; output->position[1] = position.y; output->position[2] = position.z;
    }
}

int RenderExpandNativeLocalDraw(const RageNativeDrawSpan *span,
    RageNativeGpuVertex *vertices, uint32_t capacity) {
    if (!span || !vertices || !span->localGeometry || !span->localGeometry->vertices || capacity < span->vertexCount ||
        span->localFirstVertex > span->localGeometry->vertexCount ||
        span->vertexCount > span->localGeometry->vertexCount - span->localFirstVertex) return 0;
    RageTransformBasis basis = RenderPrepareInstanceTransform(&span->localTransform);
    TransformTemplateVertices(span->localGeometry->vertices + span->localFirstVertex,
        span->vertexCount, &basis, span->instanceFlags, span->depthDecal, vertices);
    return 1;
}

static int AppendMeshTemplate(const RageNativeMeshTemplateView *source, int localDraws,
    const RageRenderMeshInstance *instance, const RageTransformBasis *basis,
    const RageNativeInstanceState *state, RageNativeGpuVertex *vertices, uint32_t capacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *vertexCount, uint32_t *spanCount) {
    for (uint32_t s = 0; s < source->spanCount; ++s) {
        const RageNativeDrawSpan *input = &source->spans[s];
        uint32_t count = input->vertexCount;
        if (count > capacity - *vertexCount) count = (capacity - *vertexCount) / 3 * 3;
        if (!count) return 1;
        if (localDraws || !*spanCount || !SpanMatches(&spans[*spanCount - 1], instance, state,
                input->material, input->materialFlags, input->depthDecal, instance->materialVariant)) {
            if (*spanCount == spanCapacity) return 0;
            RageNativeDrawSpan *output = &spans[(*spanCount)++];
            *output = *input;
            output->firstVertex = *vertexCount;
            output->vertexCount = 0;
            output->assetKey = instance->assetKey;
            output->sourceEntity = instance->entity;
            output->entity = instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ? instance->entity : 0;
            output->instanceFlags = instance->flags;
            output->materialVariant = instance->materialVariant;
            output->hasCarPaint = instance->hasCarPaint;
            output->carPaintColor1 = instance->carPaintColor1;
            output->carPaintColor2 = instance->carPaintColor2;
            output->component = instance->component;
            output->pass = instance->pass;
            output->instanceState = *state;
            if (localDraws) {
                output->localGeometry = source;
                output->localFirstVertex = input->firstVertex;
                output->localTransform = instance->transform;
            }
        }
        if (localDraws != 2)
            TransformTemplateVertices(source->vertices + input->firstVertex, count,
                basis, instance->flags, input->depthDecal, vertices + *vertexCount);
        *vertexCount += count;
        spans[*spanCount - 1].vertexCount += count;
    }
    return 1;
}

static uint32_t RenderBuildNativeDrawsFiltered(
    RageNativeMeshTemplateCache *cache, int localSource, int localDraws,
    const RageRenderWorld *world, int passFilter, float aspect, int gpuFog,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, RageNativeGpuVertex *compactVertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    uint32_t instanceIndex, vertexCount = 0, spansUsed = 0;
    /* One view/build invocation only. Epochs isolate transforms, colours and
     * scroll state of different instances sharing the same source mesh.
     * Cache the base vertex BEFORE per-triangle normals/displacement. */
    RagePreparedVertexCacheEntry vertexCache[256] = {0};
    RageRenderViewTransform viewTransform;
    RageInstanceFrustum frustum;
    if (spanCount != NULL) *spanCount = 0;
    if (world == NULL || lookup == NULL || (vertices == NULL && compactVertices == NULL) || spans == NULL ||
        spanCount == NULL || !isfinite(aspect) || aspect <= 0.0f ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCount != 0 && world->instances == NULL)) return 0;
    viewTransform = RenderPrepareView(&world->camera);
    frustum = PrepareInstanceFrustum(world, aspect);
    for (instanceIndex = 0; instanceIndex < world->instanceCount; instanceIndex++) {
        const RageRenderMeshInstance *instance = &world->instances[instanceIndex];
        const RageRuntimeMesh *mesh;
        RageTransformBasis basis;
        RageNativeInstanceState instanceState;
        uint32_t first, count, offset;
        int terrainQuadHidden = 0;
        int terrainPositionsValid = 0;
        RageRenderVec3 terrainPositions[6];
        if (passFilter >= 0 && instance->pass != (RageRenderPass)passFilter)
            continue;
        /* Excluded passes must not consult (or trigger work in) the asset
         * provider. Both cameras may consume only the main semantic scene. */
        mesh = lookup(context, instance);
        if (mesh == NULL || !RuntimeMeshRange(mesh, instance->mesh, &first, &count)) {
            continue;
        }
        basis = RenderPrepareInstanceTransform(&instance->transform);
        if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL) &&
            InstanceOutsideFrustum(world, &viewTransform, &frustum, &basis, mesh,
                                       instance->mesh)) continue;
        instanceState = PrepareInstanceState(instance);
        if (cache && gpuFog && compactVertices && !instance->textureScrollU &&
            (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
             instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) &&
            !(instance->flags & (RAGE_RENDER_INSTANCE_FLAT_SHADED |
              RAGE_RENDER_INSTANCE_DEPTH_DECAL | RAGE_RENDER_INSTANCE_CULL_BACKFACES))) {
            const RageNativeMeshTemplateView *prepared = RenderNativeMeshTemplateAcquire(
                cache, mesh, instance->assetSet, instance->mesh);
            if (prepared) {
                if (!AppendMeshTemplate(prepared, localDraws, instance, &basis, &instanceState,
                        compactVertices, vertexCapacity, spans, spanCapacity, &vertexCount, &spansUsed)) goto done;
                continue;
            }
        }
        for (offset = 0; offset + 2 < count; offset += 3) {
            RageNativeDrawVertex triangle[3];
            uint32_t materials[3], materialFlags[3], indices[3];
            uint8_t depthDecals[3];
            uint8_t materialVariant;
            uint32_t corner;
            int valid = 1;
            if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
                if ((offset % 6u) == 0) {
                    terrainQuadHidden = 0;
                    terrainPositionsValid = 0;
                    /* Global indices can continue into the next mesh range.
                     * They must not complete this range's trailing triangle. */
                    if (count - offset >= 6)
                        terrainQuadHidden = TerrainQuadIsHidden(
                            world, &basis, &viewTransform, mesh, first + offset,
                            terrainPositions, &terrainPositionsValid);
                }
                if (terrainQuadHidden) continue;
            }
            for (corner = 0; corner < 3; corner++) {
                valid = valid && RuntimeMeshIndex(mesh, first + offset + corner,
                                                      &indices[corner]);
                if (valid) {
                    RagePreparedVertexCacheEntry *entry =
                        &vertexCache[indices[corner] & 255u];
                    uint32_t epoch = instanceIndex + 1;
                    if (gpuFog && entry->instanceEpoch == epoch &&
                        entry->sourceIndex == indices[corner]) {
                        triangle[corner] = entry->vertex;
                        materials[corner] = entry->material;
                        materialFlags[corner] = entry->materialFlags;
                        depthDecals[corner] = entry->depthDecal;
                    } else {
                        valid = BuildVertex(&basis, &viewTransform, world,
                            (instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FOG) != 0,
                            gpuFog, compactVertices != NULL && gpuFog,
                            instance, &instanceState, mesh, indices[corner],
                            terrainPositionsValid ? &terrainPositions[offset % 6u + corner] : NULL,
                            aspect, localSource,
                            &triangle[corner], &materials[corner],
                            &materialFlags[corner], &depthDecals[corner]);
                        if (gpuFog && valid) {
                            entry->instanceEpoch = epoch;
                            entry->sourceIndex = indices[corner];
                            entry->vertex = triangle[corner];
                            entry->material = materials[corner];
                            entry->materialFlags = materialFlags[corner];
                            entry->depthDecal = depthDecals[corner];
                        }
                    }
                }
            }
            /* Modded triangles may mix scrolling and fixed corners. Such a
             * triangle cannot use one draw-wide offset; retain its original
             * per-corner evaluation instead of dropping valid geometry. */
            if (valid && compactVertices != NULL && gpuFog &&
                (((materialFlags[0] ^ materialFlags[1]) |
                  (materialFlags[0] ^ materialFlags[2])) &
                 RAGE_RUNTIME_MATERIAL_SCROLL_U) != 0) {
                for (corner = 0; corner < 3; ++corner) {
                    if (materialFlags[corner] & RAGE_RUNTIME_MATERIAL_SCROLL_U)
                        triangle[corner].uv[0] +=
                            (float)instance->textureScrollU * (1.0f / 256.0f);
                    materialFlags[corner] &= ~(uint32_t)RAGE_RUNTIME_MATERIAL_SCROLL_U;
                }
            }
            if (!valid ||
                materials[0] != materials[1] || materials[0] != materials[2] ||
                materialFlags[0] != materialFlags[1] ||
                materialFlags[0] != materialFlags[2] ||
                depthDecals[0] != depthDecals[1] ||
                depthDecals[0] != depthDecals[2] ||
                vertexCount > vertexCapacity ||
                vertexCapacity - vertexCount < 3) continue;
            RageTriangleGeometry geometry = {0};
            if ((instance->flags & RAGE_RENDER_INSTANCE_FLAT_SHADED) != 0)
                ApplyFlatTriangleNormal(triangle, &geometry);
            if (depthDecals[0] && !localSource) {
                /* Explicit screen/art layers are semantic overlays. Give them
                 * real separation from their backing mesh instead of changing
                 * their depth value in the rasterizer. */
                if ((instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
                     instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) &&
                    materials[0] / RAGE_CAR_SURFACE_RUNTIME_STRIDE ==
                        RAGE_CAR_SURFACE_DECAL)
                    LiftCarDecal(triangle);
                else
                    LiftOverlayTowardCamera(
                        triangle, world->camera.transform.position, &geometry);
            } else if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN &&
                materials[0] != UINT32_MAX &&
                TriangleIsRoadDecal(triangle, &geometry)) {
                LiftRoadDecal(triangle, &geometry);
                depthDecals[0] = depthDecals[1] = depthDecals[2] = 1;
            }
            if ((instance->flags & RAGE_RENDER_INSTANCE_CULL_BACKFACES) != 0 &&
                TriangleIsBackFacing(world, &viewTransform, triangle)) continue;
            instanceState.textureScrollU =
                (materialFlags[0] & RAGE_RUNTIME_MATERIAL_SCROLL_U) != 0
                ? (float)instance->textureScrollU * (1.0f / 256.0f) : 0.0f;
            materialVariant = instance->materialVariant;
            /* Only terrain modes 0/1 select CLUT+1 from environment mode 4.
             * Modes 2..5 already encode their fixed CLUT in the import.
             * Keep the track-page bit while removing that extra CLUT shift. */
            if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN &&
                (materialFlags[0] & RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT) == 0)
                materialVariant &= (uint8_t)~1u;
            if (spansUsed == 0 || spans[spansUsed - 1].localGeometry || !SpanMatches(&spans[spansUsed - 1], instance,
                    &instanceState, materials[0], materialFlags[0], depthDecals[0], materialVariant)) {
                if (spansUsed == spanCapacity) goto done;
                memset(&spans[spansUsed], 0, sizeof(spans[spansUsed]));
                spans[spansUsed].firstVertex = vertexCount;
                spans[spansUsed].vertexCount = 0;
                spans[spansUsed].assetKey = instance->assetKey;
                spans[spansUsed].assetSet = instance->assetSet;
                spans[spansUsed].mesh = instance->mesh;
                spans[spansUsed].sourceEntity = instance->entity;
                spans[spansUsed].instanceFlags = instance->flags;
                spans[spansUsed].material = materials[0];
                spans[spansUsed].materialFlags = materialFlags[0];
                spans[spansUsed].depthDecal = depthDecals[0];
                spans[spansUsed].materialVariant = materialVariant;
                spans[spansUsed].hasCarPaint = instance->hasCarPaint;
                spans[spansUsed].carPaintColor1 = instance->carPaintColor1;
                spans[spansUsed].carPaintColor2 = instance->carPaintColor2;
                spans[spansUsed].component = instance->component;
                /* Course and terrain share immutable materials. Only model
                 * banks can carry an entity-specific material variant (car
                 * paint), so do not explode the texture cache per cell. */
                spans[spansUsed].entity =
                    instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK
                    ? instance->entity : 0;
                spans[spansUsed].pass = instance->pass;
                spans[spansUsed].instanceState = instanceState;
                spansUsed++;
            }
            if (compactVertices != NULL) {
                for (corner = 0; corner < 3; ++corner)
                    compactVertices[vertexCount + corner] = RenderPackNativeGpuVertex(&triangle[corner]);
            } else {
                memcpy(&vertices[vertexCount], triangle, sizeof(triangle));
            }
            vertexCount += 3;
            spans[spansUsed - 1].vertexCount += 3;
        }
    }
done:
    *spanCount = spansUsed;
    return vertexCount;
}

void RenderNativeMeshTemplateCacheRelease(RageNativeMeshTemplateCache *cache) {
    if (!cache || !cache->state) return;
    RageNativeMeshTemplateState *state = cache->state;
    while (state->first) {
        RageNativeMeshTemplate *entry = state->first;
        state->first = entry->next;
        free(entry->vertices); free(entry->spans); free(entry);
    }
    free(state);
    cache->state = NULL;
}

static uint32_t ViewRangeHash(const RageNativeGpuVertex *vertex, uint32_t count) {
    uint32_t hash = count;
    const unsigned char *bytes = (const unsigned char *)vertex;
    for (size_t i = 0; i < sizeof(*vertex); ++i) hash = (hash ^ bytes[i]) * 16777619u;
    return hash ^ (hash >> 16);
}

uint32_t RenderShareNativeViewVertices(RageNativeGpuVertex *vertices,
    uint32_t mainCount, const RageNativeDrawSpan *mainSpans, uint32_t mainSpanCount,
    uint32_t mirrorCount, RageNativeDrawSpan *mirrorSpans, uint32_t mirrorSpanCount) {
    enum { SLOTS = 4096, PROBES = 8 };
    uint32_t slots[SLOTS] = {0};
    if (mirrorCount > UINT32_MAX - mainCount) return UINT32_MAX;
    const uint32_t total = mainCount + mirrorCount;
    if (!vertices || !mainSpans || !mirrorSpans || !mainCount || !mirrorCount) return total;
    for (uint32_t i = 0; i < mainSpanCount; ++i)
        if (mainSpans[i].firstVertex > mainCount ||
            mainSpans[i].vertexCount > mainCount - mainSpans[i].firstVertex) return total;
    uint32_t cursor = mainCount;
    for (uint32_t i = 0; i < mirrorSpanCount; ++i) {
        if (mirrorSpans[i].firstVertex != cursor || mirrorSpans[i].vertexCount > total - cursor)
            return total;
        cursor += mirrorSpans[i].vertexCount;
    }
    if (cursor != total) return total;
    for (uint32_t i = 0; i < mainSpanCount; ++i) {
        const RageNativeDrawSpan *span = &mainSpans[i];
        if (!span->vertexCount) continue;
        uint32_t slot = ViewRangeHash(vertices + span->firstVertex, span->vertexCount) & (SLOTS - 1);
        for (unsigned probe = 0; probe < PROBES; ++probe, slot = (slot + 1) & (SLOTS - 1)) {
            if (!slots[slot]) { slots[slot] = i + 1; break; }
        }
    }
    cursor = mainCount;
    for (uint32_t i = 0; i < mirrorSpanCount; ++i) {
        RageNativeDrawSpan *span = &mirrorSpans[i];
        const RageNativeGpuVertex *source = vertices + span->firstVertex;
        uint32_t first = cursor;
        int shared = 0;
        if (span->vertexCount) {
            uint32_t slot = ViewRangeHash(source, span->vertexCount) & (SLOTS - 1);
            for (unsigned probe = 0; probe < PROBES && slots[slot]; ++probe, slot = (slot + 1) & (SLOTS - 1)) {
                const RageNativeDrawSpan *candidate = &mainSpans[slots[slot] - 1];
                if (candidate->vertexCount == span->vertexCount &&
                    !memcmp(vertices + candidate->firstVertex, source,
                        (size_t)span->vertexCount * sizeof(*vertices))) {
                    first = candidate->firstVertex;
                    shared = 1;
                    break;
                }
            }
        }
        if (!shared) {
            memmove(vertices + cursor, source, (size_t)span->vertexCount * sizeof(*vertices));
            cursor += span->vertexCount;
        }
        span->firstVertex = first;
    }
    return cursor;
}

const RageNativeMeshTemplateView *RenderNativeMeshTemplateAcquire(
    RageNativeMeshTemplateCache *cache, const RageRuntimeMesh *mesh,
    RageRenderAssetSet assetSet, uint32_t submesh) {
    enum { MAX_BYTES = 32 * 1024 * 1024 };
    uint32_t first, count;
    if (!cache || !mesh ||
        (assetSet != RAGE_RENDER_ASSET_MODEL_BANK &&
         assetSet != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) ||
        !RuntimeMeshRange(mesh, submesh, &first, &count) || count < 3) return NULL;
    if (!cache->state) cache->state = calloc(1, sizeof(RageNativeMeshTemplateState));
    RageNativeMeshTemplateState *state = cache->state;
    if (!state) return NULL;
    for (RageNativeMeshTemplate *entry = state->first; entry; entry = entry->next)
        if (entry->source == mesh && entry->mesh == submesh && entry->assetSet == assetSet)
            return &entry->view;
    if (count > MAX_BYTES / (sizeof(RageNativeGpuVertex) + sizeof(RageNativeDrawSpan))) return NULL;
    size_t bytes = (size_t)count * sizeof(RageNativeGpuVertex) +
                  (size_t)(count / 3) * sizeof(RageNativeDrawSpan) + sizeof(RageNativeMeshTemplate);
    if (bytes > MAX_BYTES - state->bytes) return NULL;
    RageNativeMeshTemplate *entry = calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    entry->vertices = malloc((size_t)count * sizeof(*entry->vertices));
    entry->spans = calloc(count / 3, sizeof(*entry->spans));
    if (!entry->vertices || !entry->spans) {
        free(entry->vertices); free(entry->spans); free(entry);
        return NULL;
    }
    RageRenderMeshInstance local = {0};
    RageRenderWorld world = {0};
    local.assetSet = assetSet;
    local.mesh = submesh;
    local.pass = RAGE_RENDER_PASS_MAIN;
    local.transform.scale = (RageRenderVec3){1, 1, 1};
    world.instances = &local;
    world.instanceCapacity = world.instanceCount = 1;
    world.camera.verticalFovDegrees = 90;
    world.camera.nearPlane = 1;
    world.camera.farPlane = 10000;
    entry->source = mesh;
    entry->mesh = submesh;
    entry->assetSet = assetSet;
    entry->vertexCount = RenderBuildNativeDrawsFiltered(NULL, 1, 0, &world,
        RAGE_RENDER_PASS_MAIN, 1, 1, TemplateMeshLookup, (void *)mesh, NULL,
        entry->vertices, count, entry->spans, count / 3, &entry->spanCount);
    entry->next = state->first;
    state->first = entry;
    state->bytes += bytes;
    entry->view = (RageNativeMeshTemplateView){entry->vertices, entry->spans,
        entry->vertexCount, entry->spanCount};
    return &entry->view;
}

uint32_t RenderBuildNativeDraws(const RageRenderWorld *world, float aspect,
                                    RageRenderMeshLookup lookup, void *context,
                                    RageNativeDrawVertex *vertices,
                                    uint32_t vertexCapacity,
                                    RageNativeDrawSpan *spans,
                                    uint32_t spanCapacity,
                                    uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        NULL, 0, 0, world, -1, aspect, 0, lookup, context, vertices, NULL, vertexCapacity, spans,
        spanCapacity, spanCount);
}

uint32_t RenderBuildNativePassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        NULL, 0, 0, world, (int)pass, aspect, 0, lookup, context, vertices, NULL, vertexCapacity,
        spans, spanCapacity, spanCount);
}

uint32_t RenderBuildNativeGpuPassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        NULL, 0, 0, world, (int)pass, aspect, 1, lookup, context, vertices, NULL, vertexCapacity,
        spans, spanCapacity, spanCount);
}

uint32_t RenderBuildNativeCompactPassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(NULL, 0, 0, world, (int)pass, aspect, !cpuFog,
        lookup, context, NULL, vertices, vertexCapacity, spans, spanCapacity, spanCount);
}

uint32_t RenderBuildNativeCachedCompactPassDraws(
    RageNativeMeshTemplateCache *cache,
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(cache, 0, 0, world, (int)pass, aspect, !cpuFog,
        lookup, context, NULL, vertices, vertexCapacity, spans, spanCapacity, spanCount);
}

uint32_t RenderBuildNativeLocalCompactPassDraws(
    RageNativeMeshTemplateCache *cache,
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog, int expandWorldVertices,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(cache, 0, expandWorldVertices ? 1 : 2, world, (int)pass, aspect, !cpuFog,
        lookup, context, NULL, vertices, vertexCapacity, spans, spanCapacity, spanCount);
}
