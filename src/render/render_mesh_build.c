#include "render_mesh_build.h"
#include "render_native_vertex.h"
#include "render_instance_transform.h"
#include "render_triangle_geometry.h"
#include "authored_car_surface.h"

#include <math.h>
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
    const RageRuntimeMesh *mesh, uint32_t first) {
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

static int InstanceOutsideFrustum(const RageRenderWorld *world,
                                      const RageRenderViewTransform *viewTransform,
                                      const RageRenderTransform *transform,
                                      const RageRuntimeMesh *mesh,
                                      uint32_t meshIndex, float aspect) {
    float center[3], radius, maxScale, tanY, tanX, depth;
    float horizontalRadius, verticalRadius;
    RageRenderVec3 worldCenter, view;
    RageTransformBasis basis = RenderPrepareInstanceTransform(transform);
    if (!RuntimeMeshBounds(mesh, meshIndex, center, &radius)) return 0;
    worldCenter = TransformPoint(&basis, center);
    RenderWorldToViewPrepared(viewTransform, &worldCenter, &view);
    depth = -view.z;
    maxScale = fmaxf(fabsf(transform->scale.x),
                     fmaxf(fabsf(transform->scale.y), fabsf(transform->scale.z)));
    radius *= maxScale;
    if (depth + radius < world->camera.nearPlane ||
        depth - radius > world->camera.farPlane) return 1;
    tanY = tanf(Radians(world->camera.verticalFovDegrees) * 0.5f);
    tanX = tanY * aspect;
    /* Keep a small guard band around the visible frustum. In a low cockpit
     * camera the road can cross the side plane between logic ticks on a
     * sharp bend; exact-edge culling otherwise exposes a one-cell notch for
     * a frame before the interpolated camera catches up. */
    tanX *= 1.08f;
    tanY *= 1.08f;
    /* Test the sphere against the actual side planes. Comparing its
     * axis-aligned radius with the frustum width at the sphere centre is not
     * conservative: a large nearby terrain cell can cross a side plane even
     * when its centre is well outside it. */
    horizontalRadius = radius * sqrtf(1.0f + tanX * tanX);
    verticalRadius = radius * sqrtf(1.0f + tanY * tanY);
    return fabsf(view.x) > depth * tanX + horizontalRadius ||
           fabsf(view.y) > depth * tanY + verticalRadius;
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
                           float aspect, RageNativeDrawVertex *out,
                           uint32_t *material, uint32_t *materialFlags,
                           uint8_t *depthDecal) {
    RageRuntimeVertex source;
    RageRenderVec3 normal;
    RageRenderVec3 worldPosition;
    if (!RuntimeMeshVertex(mesh, index, &source)) return 0;
    *materialFlags = source.material &
        (RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY |
         RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT);
    worldPosition = TransformPosition(basis, &source);
    if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
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
    normal = TransformNormal(basis, &source);
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

static uint32_t RenderBuildNativeDrawsFiltered(
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
    if (spanCount != NULL) *spanCount = 0;
    if (world == NULL || lookup == NULL || (vertices == NULL && compactVertices == NULL) || spans == NULL ||
        spanCount == NULL || !isfinite(aspect) || aspect <= 0.0f ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCount != 0 && world->instances == NULL)) return 0;
    viewTransform = RenderPrepareView(&world->camera);
    for (instanceIndex = 0; instanceIndex < world->instanceCount; instanceIndex++) {
        const RageRenderMeshInstance *instance = &world->instances[instanceIndex];
        const RageRuntimeMesh *mesh;
        RageTransformBasis basis;
        RageNativeInstanceState instanceState;
        uint32_t first, count, offset;
        int terrainQuadHidden = 0;
        if (passFilter >= 0 && instance->pass != (RageRenderPass)passFilter)
            continue;
        /* Excluded passes must not consult (or trigger work in) the asset
         * provider. Both cameras may consume only the main semantic scene. */
        mesh = lookup(context, instance);
        if (mesh == NULL || !RuntimeMeshRange(mesh, instance->mesh, &first, &count)) {
            continue;
        }
        if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL) &&
            InstanceOutsideFrustum(world, &viewTransform, &instance->transform, mesh,
                                       instance->mesh, aspect)) continue;
        basis = RenderPrepareInstanceTransform(&instance->transform);
        instanceState = PrepareInstanceState(instance);
        for (offset = 0; offset + 2 < count; offset += 3) {
            RageNativeDrawVertex triangle[3];
            uint32_t materials[3], materialFlags[3], indices[3];
            uint8_t depthDecals[3];
            uint8_t materialVariant;
            uint32_t corner;
            int valid = 1;
            if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
                if ((offset % 6u) == 0)
                    terrainQuadHidden = TerrainQuadIsHidden(
                        world, &basis, &viewTransform, mesh, first + offset);
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
                            instance, &instanceState, mesh, indices[corner], aspect,
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
            if (depthDecals[0]) {
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
            if (spansUsed == 0 || spans[spansUsed - 1].material != materials[0] ||
                spans[spansUsed - 1].materialFlags != materialFlags[0] ||
                spans[spansUsed - 1].depthDecal != depthDecals[0] ||
                spans[spansUsed - 1].assetKey != instance->assetKey ||
                spans[spansUsed - 1].assetSet != instance->assetSet ||
                spans[spansUsed - 1].mesh != instance->mesh ||
                spans[spansUsed - 1].sourceEntity != instance->entity ||
                spans[spansUsed - 1].instanceFlags != instance->flags ||
                memcmp(&spans[spansUsed - 1].instanceState, &instanceState,
                       sizeof(instanceState)) != 0 ||
                spans[spansUsed - 1].materialVariant != materialVariant ||
                spans[spansUsed - 1].hasCarPaint != instance->hasCarPaint ||
                spans[spansUsed - 1].carPaintColor1 != instance->carPaintColor1 ||
                spans[spansUsed - 1].carPaintColor2 != instance->carPaintColor2 ||
                spans[spansUsed - 1].component != instance->component ||
                spans[spansUsed - 1].entity !=
                    (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK
                     ? instance->entity : 0) ||
                spans[spansUsed - 1].pass != instance->pass) {
                if (spansUsed == spanCapacity) goto done;
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

uint32_t RenderBuildNativeDraws(const RageRenderWorld *world, float aspect,
                                    RageRenderMeshLookup lookup, void *context,
                                    RageNativeDrawVertex *vertices,
                                    uint32_t vertexCapacity,
                                    RageNativeDrawSpan *spans,
                                    uint32_t spanCapacity,
                                    uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        world, -1, aspect, 0, lookup, context, vertices, NULL, vertexCapacity, spans,
        spanCapacity, spanCount);
}

uint32_t RenderBuildNativePassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        world, (int)pass, aspect, 0, lookup, context, vertices, NULL, vertexCapacity,
        spans, spanCapacity, spanCount);
}

uint32_t RenderBuildNativeGpuPassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        world, (int)pass, aspect, 1, lookup, context, vertices, NULL, vertexCapacity,
        spans, spanCapacity, spanCount);
}

uint32_t RenderBuildNativeCompactPassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect, int cpuFog,
    RageRenderMeshLookup lookup, void *context,
    RageNativeGpuVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(world, (int)pass, aspect, !cpuFog,
        lookup, context, NULL, vertices, vertexCapacity, spans, spanCapacity, spanCount);
}
