#include "render_mesh_build.h"

#include <math.h>
#include <string.h>

static float RageRadians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

typedef struct RageTransformBasis {
    RageRenderVec3 position;
    RageRenderVec3 scale;
    float cx, sx, cy, sy, cz, sz;
    float matrix[3][3];
    int useMatrix;
} RageTransformBasis;

static RageTransformBasis RageBuildTransformBasis(const RageRenderTransform *transform) {
    RageTransformBasis basis = {0};
    float x = RageRadians(transform->rotation.x);
    float y = RageRadians(transform->rotation.y);
    float z = RageRadians(transform->rotation.z);
    basis.position = transform->position;
    basis.scale = transform->scale;
    basis.cx = cosf(x); basis.sx = sinf(x);
    basis.cy = cosf(y); basis.sy = sinf(y);
    basis.cz = cosf(z); basis.sz = sinf(z);
    if (transform->hasOrientation) {
        const RageRenderQuaternion *q = &transform->orientation;
        float length = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
        if (length > 0.0f) {
            float xq = q->x / length, yq = q->y / length;
            float zq = q->z / length, wq = q->w / length;
            basis.matrix[0][0] = 1.0f - 2.0f * (yq * yq + zq * zq);
            basis.matrix[0][1] = 2.0f * (xq * yq - zq * wq);
            basis.matrix[0][2] = 2.0f * (xq * zq + yq * wq);
            basis.matrix[1][0] = 2.0f * (xq * yq + zq * wq);
            basis.matrix[1][1] = 1.0f - 2.0f * (xq * xq + zq * zq);
            basis.matrix[1][2] = 2.0f * (yq * zq - xq * wq);
            basis.matrix[2][0] = 2.0f * (xq * zq - yq * wq);
            basis.matrix[2][1] = 2.0f * (yq * zq + xq * wq);
            basis.matrix[2][2] = 1.0f - 2.0f * (xq * xq + yq * yq);
            basis.useMatrix = 1;
        }
    }
    return basis;
}

static RageRenderVec3 RageTransformBasisVector(const RageTransformBasis *basis,
                                                RageRenderVec3 out) {
    float x;
    if (basis->useMatrix) {
        RageRenderVec3 rotated;
        rotated.x = basis->matrix[0][0] * out.x + basis->matrix[0][1] * out.y +
                    basis->matrix[0][2] * out.z;
        rotated.y = basis->matrix[1][0] * out.x + basis->matrix[1][1] * out.y +
                    basis->matrix[1][2] * out.z;
        rotated.z = basis->matrix[2][0] * out.x + basis->matrix[2][1] * out.y +
                    basis->matrix[2][2] * out.z;
        return rotated;
    }
    float y = out.y * basis->cx - out.z * basis->sx;
    float z = out.y * basis->sx + out.z * basis->cx;
    out.y = y; out.z = z;
    x = out.x * basis->cy + out.z * basis->sy;
    z = -out.x * basis->sy + out.z * basis->cy;
    out.x = x; out.z = z;
    x = out.x * basis->cz - out.y * basis->sz;
    y = out.x * basis->sz + out.y * basis->cz;
    out.x = x; out.y = y;
    return out;
}

static RageRenderVec3 RageTransformPosition(const RageTransformBasis *basis,
                                             const RageRuntimeVertex *vertex) {
    RageRenderVec3 out = {vertex->position[0] * basis->scale.x,
                          vertex->position[1] * basis->scale.y,
                          vertex->position[2] * basis->scale.z};
    out = RageTransformBasisVector(basis, out);
    out.x += basis->position.x;
    out.y += basis->position.y;
    out.z += basis->position.z;
    return out;
}

static float RageSnapTerrainCellBoundary(float value) {
    const float cellSize = 2048.0f;
    float boundary = roundf(value / cellSize) * cellSize;
    /* Adjacent PS1 terrain cells occasionally disagree by one source GTE
     * unit (0.25 native world units). The original low-resolution rasterizer
     * hid that quantization gap; weld only vertices already on a cell edge. */
    return fabsf(value - boundary) <= 0.5f ? boundary : value;
}

static RageRenderVec3 RageTransformNormal(const RageTransformBasis *basis,
                                          const RageRuntimeVertex *vertex) {
    RageRenderVec3 out = {vertex->normal[0], vertex->normal[1], vertex->normal[2]};
    return RageTransformBasisVector(basis, out);
}

static RageRenderVec3 RageTransformPoint(const RageTransformBasis *basis,
                                         const float position[3]) {
    RageRuntimeVertex vertex = {0};
    memcpy(vertex.position, position, sizeof(vertex.position));
    return RageTransformPosition(basis, &vertex);
}

static int RageTriangleIsBackFacing(const RageRenderWorld *world,
                                    const RageNativeDrawVertex triangle[3]) {
    RageRenderVec3 view[3];
    float screenX[3], screenY[3];
    int corner;
    for (corner = 0; corner < 3; corner++) {
        RageRenderVec3 position = {triangle[corner].position[0],
                                   triangle[corner].position[1],
                                   triangle[corner].position[2]};
        float depth;
        RageRenderWorldToView(&world->camera, &position, &view[corner]);
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

static int RageInstanceOutsideFrustum(const RageRenderWorld *world,
                                      const RageRenderTransform *transform,
                                      const RageRuntimeMesh *mesh,
                                      uint32_t meshIndex, float aspect) {
    float center[3], radius, maxScale, tanY, tanX, depth;
    float horizontalRadius, verticalRadius;
    RageRenderVec3 worldCenter, view;
    RageTransformBasis basis = RageBuildTransformBasis(transform);
    if (!RageRuntimeMeshBounds(mesh, meshIndex, center, &radius)) return 0;
    worldCenter = RageTransformPoint(&basis, center);
    RageRenderWorldToView(&world->camera, &worldCenter, &view);
    depth = -view.z;
    maxScale = fmaxf(fabsf(transform->scale.x),
                     fmaxf(fabsf(transform->scale.y), fabsf(transform->scale.z)));
    radius *= maxScale;
    if (depth + radius < world->camera.nearPlane ||
        depth - radius > world->camera.farPlane) return 1;
    tanY = tanf(RageRadians(world->camera.verticalFovDegrees) * 0.5f);
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

static int RageBuildVertex(const RageTransformBasis *basis,
                           const RageRenderWorld *world, int fogged,
                           const RageRenderMeshInstance *instance,
                           const RageRuntimeMesh *mesh, uint32_t index,
                           float aspect, RageNativeDrawVertex *out,
                           uint32_t *material, uint32_t *materialFlags,
                           uint8_t *depthDecal) {
    RageRuntimeVertex source;
    RageRenderVec3 worldPosition;
    if (!RageRuntimeMeshVertex(mesh, index, &source)) return 0;
    *materialFlags = source.material &
        (RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY |
         RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT |
         RAGE_RUNTIME_MATERIAL_TERRAIN_LINE);
    if ((source.material & RAGE_RUNTIME_MATERIAL_TERRAIN_LINE) != 0)
        *materialFlags |= source.material & 0xFFu;
    worldPosition = RageTransformPosition(basis, &source);
    if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
        worldPosition.x = RageSnapTerrainCellBoundary(worldPosition.x);
        worldPosition.z = RageSnapTerrainCellBoundary(worldPosition.z);
    }
    (void)aspect;
    out->position[0] = worldPosition.x; out->position[1] = worldPosition.y;
    out->position[2] = worldPosition.z;
    out->uv[0] = source.uv[0]; out->uv[1] = source.uv[1];
    if (source.material != UINT32_MAX &&
        (source.material & RAGE_RUNTIME_MATERIAL_SCROLL_U) != 0) {
        out->uv[0] += (float)instance->textureScrollU * (1.0f / 256.0f);
        source.material &= ~RAGE_RUNTIME_MATERIAL_SCROLL_U;
    }
    memcpy(out->color, source.color, sizeof(out->color));
    {
        RageRenderVec3 normal = RageTransformNormal(basis, &source);
        out->normal[0] = normal.x; out->normal[1] = normal.y;
        out->normal[2] = normal.z;
    }
    out->fog[0] = world->camera.fogColor.x;
    out->fog[1] = world->camera.fogColor.y;
    out->fog[2] = world->camera.fogColor.z;
    out->fog[3] = fogged
        ? RageRenderFogFactor(&world->camera, &worldPosition) : 0.0f;
    out->lighting =
        (instance->flags & RAGE_RENDER_INSTANCE_ENABLE_LIGHTING) != 0
        ? 1.0f : 0.0f;
    out->environmentLight[0] = instance->environmentLight.x;
    out->environmentLight[1] = instance->environmentLight.y;
    out->environmentLight[2] = instance->environmentLight.z;
    if (out->environmentLight[0] == 0.0f &&
        out->environmentLight[1] == 0.0f &&
        out->environmentLight[2] == 0.0f) {
        /* Zero-initialized callers predate environment lighting. Preserve
         * their neutral light instead of turning them black. */
        out->environmentLight[0] = 1.0f;
        out->environmentLight[1] = 1.0f;
        out->environmentLight[2] = 1.0f;
    }
    out->depthBias = instance->depthBias;
    if ((*materialFlags & RAGE_RUNTIME_MATERIAL_TERRAIN_LINE) != 0) {
        /* Authored LINE_F3 edges are expanded into ordinary triangles by the
         * importer. They still lie exactly on their parent road face, so a
         * deterministic clip-space tie-break is required in addition to the
         * backend's rasterizer bias. Without it, Metal resolves alternating
         * pixels to the asphalt at grazing angles and the stripe appears to
         * shorten or disappear. Vehicles are rendered after this phase and
         * remain protected by the real depth buffer. */
        out->depthBias -= 256.0f;
    }
    out->shadowReception =
        instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
        instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1
        ? 0.0f : 1.0f;
    *depthDecal =
        (instance->flags & RAGE_RENDER_INSTANCE_DEPTH_DECAL) != 0;
    if ((source.material & RAGE_RUNTIME_MATERIAL_METADATA) != 0) {
        int8_t authoredDepthBias = (int8_t)(source.material >>
            RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT);
        /* Terrain OT bias only identifies coplanar details such as road
         * markings. Carrying its numeric value into a Z buffer lets distant
         * markings jump in front of cars, so the decal pipeline owns their
         * tiny depth tie-break instead. Non-terrain assets still use the bias
         * for authored overlaps such as car body and wheel geometry. */
        if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
            if (authoredDepthBias < 0) *depthDecal = 1;
        } else {
            out->depthBias += (float)authoredDepthBias;
        }
        source.material &= RAGE_RUNTIME_MATERIAL_INDEX_MASK;
        if ((*materialFlags & RAGE_RUNTIME_MATERIAL_TERRAIN_LINE) != 0 ||
            source.material == RAGE_RUNTIME_MATERIAL_INDEX_MASK)
            source.material = UINT32_MAX;
    }
    *material = source.material;
    return 1;
}

static uint32_t RageRenderBuildNativeDrawsFiltered(
    const RageRenderWorld *world, int passFilter, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    uint32_t instanceIndex, vertexCount = 0, spansUsed = 0;
    if (spanCount != NULL) *spanCount = 0;
    if (world == NULL || lookup == NULL || vertices == NULL || spans == NULL ||
        spanCount == NULL) return 0;
    for (instanceIndex = 0; instanceIndex < world->instanceCount; instanceIndex++) {
        const RageRenderMeshInstance *instance = &world->instances[instanceIndex];
        const RageRuntimeMesh *mesh = lookup(context, instance);
        RageTransformBasis basis;
        uint32_t first, count, offset;
        if (passFilter >= 0 && instance->pass != (RageRenderPass)passFilter)
            continue;
        if (mesh == NULL || !RageRuntimeMeshRange(mesh, instance->mesh, &first, &count)) {
            continue;
        }
        if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL) &&
            RageInstanceOutsideFrustum(world, &instance->transform, mesh,
                                       instance->mesh, aspect)) continue;
        basis = RageBuildTransformBasis(&instance->transform);
        for (offset = 0; offset + 2 < count; offset += 3) {
            RageNativeDrawVertex triangle[3];
            uint32_t materials[3], materialFlags[3], indices[3];
            uint8_t depthDecals[3];
            uint32_t corner;
            int valid = 1;
            for (corner = 0; corner < 3; corner++) {
                valid = valid && RageRuntimeMeshIndex(mesh, first + offset + corner,
                                                      &indices[corner]);
                if (valid) valid = RageBuildVertex(&basis, world,
                    (instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FOG) != 0,
                    instance, mesh, indices[corner], aspect,
                    &triangle[corner], &materials[corner],
                    &materialFlags[corner], &depthDecals[corner]);
            }
            if (!valid ||
                materials[0] != materials[1] || materials[0] != materials[2] ||
                materialFlags[0] != materialFlags[1] ||
                materialFlags[0] != materialFlags[2] ||
                depthDecals[0] != depthDecals[1] ||
                depthDecals[0] != depthDecals[2] ||
                vertexCount + 3 > vertexCapacity) continue;
            if ((materialFlags[0] &
                 RAGE_RUNTIME_MATERIAL_TERRAIN_LINE) != 0) {
                RageRenderVec3 view;
                RageRenderVec3 midpoint = {
                    (triangle[0].position[0] + triangle[1].position[0] +
                     triangle[2].position[0]) / 3.0f,
                    (triangle[0].position[1] + triangle[1].position[1] +
                     triangle[2].position[1]) / 3.0f,
                    (triangle[0].position[2] + triangle[1].position[2] +
                     triangle[2].position[2]) / 3.0f,
                };
                float authoredLevel = (float)(materialFlags[0] & 0xFFu);
                RageRenderWorldToView(&world->camera, &midpoint, &view);
                /* Retail emits these LINE_F3 edges only while their parent
                 * terrain face is subdividing: level - (OTZ >> shift) > 0.
                 * Native world units match that AVSZ4 OTZ. Terrain setup
                 * authors a shift of ten, so retain the authored near range
                 * without inheriting the screen-space PS1 line primitive. */
                if (authoredLevel <= 0.0f ||
                    -view.z >= authoredLevel * 1024.0f)
                    continue;
            }
            if ((instance->flags & RAGE_RENDER_INSTANCE_CULL_BACKFACES) != 0 &&
                RageTriangleIsBackFacing(world, triangle)) continue;
            if (spansUsed == 0 || spans[spansUsed - 1].material != materials[0] ||
                spans[spansUsed - 1].materialFlags != materialFlags[0] ||
                spans[spansUsed - 1].depthDecal != depthDecals[0] ||
                spans[spansUsed - 1].assetKey != instance->assetKey ||
                spans[spansUsed - 1].assetSet != instance->assetSet ||
                spans[spansUsed - 1].mesh != instance->mesh ||
                spans[spansUsed - 1].sourceEntity != instance->entity ||
                spans[spansUsed - 1].instanceFlags != instance->flags ||
                spans[spansUsed - 1].materialVariant != instance->materialVariant ||
                spans[spansUsed - 1].hasCarPaint != instance->hasCarPaint ||
                spans[spansUsed - 1].carPaintColor1 != instance->carPaintColor1 ||
                spans[spansUsed - 1].carPaintColor2 != instance->carPaintColor2 ||
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
                spans[spansUsed].materialVariant = instance->materialVariant;
                spans[spansUsed].hasCarPaint = instance->hasCarPaint;
                spans[spansUsed].carPaintColor1 = instance->carPaintColor1;
                spans[spansUsed].carPaintColor2 = instance->carPaintColor2;
                /* Course and terrain share immutable materials. Only model
                 * banks can carry an entity-specific material variant (car
                 * paint), so do not explode the texture cache per cell. */
                spans[spansUsed].entity =
                    instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK
                    ? instance->entity : 0;
                spans[spansUsed].pass = instance->pass;
                spansUsed++;
            }
            memcpy(&vertices[vertexCount], triangle, sizeof(triangle));
            vertexCount += 3;
            spans[spansUsed - 1].vertexCount += 3;
        }
    }
done:
    *spanCount = spansUsed;
    return vertexCount;
}

uint32_t RageRenderBuildNativeDraws(const RageRenderWorld *world, float aspect,
                                    RageRenderMeshLookup lookup, void *context,
                                    RageNativeDrawVertex *vertices,
                                    uint32_t vertexCapacity,
                                    RageNativeDrawSpan *spans,
                                    uint32_t spanCapacity,
                                    uint32_t *spanCount) {
    return RageRenderBuildNativeDrawsFiltered(
        world, -1, aspect, lookup, context, vertices, vertexCapacity, spans,
        spanCapacity, spanCount);
}

uint32_t RageRenderBuildNativePassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RageRenderBuildNativeDrawsFiltered(
        world, (int)pass, aspect, lookup, context, vertices, vertexCapacity,
        spans, spanCapacity, spanCount);
}
