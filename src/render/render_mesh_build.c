#include "render_mesh_build.h"

#include <math.h>
#include <string.h>

static float Radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

typedef struct RageTransformBasis {
    RageRenderVec3 position;
    RageRenderVec3 scale;
    float cx, sx, cy, sy, cz, sz;
    float matrix[3][3];
    int useMatrix;
} RageTransformBasis;

static RageTransformBasis BuildTransformBasis(
    const RageRenderTransform *transform) {
    RageTransformBasis basis = {0};
    float x = Radians(transform->rotation.x);
    float y = Radians(transform->rotation.y);
    float z = Radians(transform->rotation.z);
    basis.position = transform->position;
    basis.scale = transform->scale;
    basis.cx = cosf(x); basis.sx = sinf(x);
    basis.cy = cosf(y); basis.sy = sinf(y);
    basis.cz = cosf(z); basis.sz = sinf(z);
    if (transform->hasOrientation) {
        const RageRenderQuaternion *q = &transform->orientation;
        double lengthSquared =
            (double)q->x * q->x + (double)q->y * q->y +
            (double)q->z * q->z + (double)q->w * q->w;
        if (isfinite(lengthSquared) && lengthSquared > 0.0) {
            double inverseLength = 1.0 / sqrt(lengthSquared);
            float xq = (float)((double)q->x * inverseLength);
            float yq = (float)((double)q->y * inverseLength);
            float zq = (float)((double)q->z * inverseLength);
            float wq = (float)((double)q->w * inverseLength);
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

static RageRenderVec3 TransformBasisVector(
    const RageTransformBasis *basis, RageRenderVec3 vector) {
    float x;
    if (basis->useMatrix) {
        RageRenderVec3 rotated;
        rotated.x = basis->matrix[0][0] * vector.x +
                    basis->matrix[0][1] * vector.y +
                    basis->matrix[0][2] * vector.z;
        rotated.y = basis->matrix[1][0] * vector.x +
                    basis->matrix[1][1] * vector.y +
                    basis->matrix[1][2] * vector.z;
        rotated.z = basis->matrix[2][0] * vector.x +
                    basis->matrix[2][1] * vector.y +
                    basis->matrix[2][2] * vector.z;
        return rotated;
    }
    float y = vector.y * basis->cx - vector.z * basis->sx;
    float z = vector.y * basis->sx + vector.z * basis->cx;
    vector.y = y;
    vector.z = z;
    x = vector.x * basis->cy + vector.z * basis->sy;
    z = -vector.x * basis->sy + vector.z * basis->cy;
    vector.x = x;
    vector.z = z;
    x = vector.x * basis->cz - vector.y * basis->sz;
    y = vector.x * basis->sz + vector.y * basis->cz;
    vector.x = x;
    vector.y = y;
    return vector;
}

static RageRenderVec3 TransformPosition(const RageTransformBasis *basis,
                                             const RageRuntimeVertex *vertex) {
    RageRenderVec3 out = {vertex->position[0] * basis->scale.x,
                          vertex->position[1] * basis->scale.y,
                          vertex->position[2] * basis->scale.z};
    out = TransformBasisVector(basis, out);
    out.x += basis->position.x;
    out.y += basis->position.y;
    out.z += basis->position.z;
    return out;
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
    return TransformBasisVector(basis, out);
}

static RageRenderVec3 TransformPoint(const RageTransformBasis *basis,
                                         const float position[3]) {
    RageRuntimeVertex vertex = {0};
    memcpy(vertex.position, position, sizeof(vertex.position));
    return TransformPosition(basis, &vertex);
}

static float Vec3Length(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

static void ApplyFlatTriangleNormal(RageNativeDrawVertex triangle[3]) {
    float ax = triangle[1].position[0] - triangle[0].position[0];
    float ay = triangle[1].position[1] - triangle[0].position[1];
    float az = triangle[1].position[2] - triangle[0].position[2];
    float bx = triangle[2].position[0] - triangle[0].position[0];
    float by = triangle[2].position[1] - triangle[0].position[1];
    float bz = triangle[2].position[2] - triangle[0].position[2];
    float nx = ay * bz - az * by;
    float ny = az * bx - ax * bz;
    float nz = ax * by - ay * bx;
    float length = Vec3Length(nx, ny, nz);
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
static int TriangleIsRoadDecal(const RageNativeDrawVertex triangle[3]) {
    float edge[3], ax, ay, az, bx, by, bz, nx, ny, nz, normalLength;
    float shortest, longest;
    int corner;
    for (corner = 0; corner < 3; corner++) {
        int next = (corner + 1) % 3;
        float x = triangle[next].position[0] - triangle[corner].position[0];
        float y = triangle[next].position[1] - triangle[corner].position[1];
        float z = triangle[next].position[2] - triangle[corner].position[2];
        edge[corner] = Vec3Length(x, y, z);
    }
    shortest = fminf(edge[0], fminf(edge[1], edge[2]));
    longest = fmaxf(edge[0], fmaxf(edge[1], edge[2]));
    if (shortest > 16.0f || longest < 64.0f || longest < shortest * 8.0f)
        return 0;
    ax = triangle[1].position[0] - triangle[0].position[0];
    ay = triangle[1].position[1] - triangle[0].position[1];
    az = triangle[1].position[2] - triangle[0].position[2];
    bx = triangle[2].position[0] - triangle[0].position[0];
    by = triangle[2].position[1] - triangle[0].position[1];
    bz = triangle[2].position[2] - triangle[0].position[2];
    nx = ay * bz - az * by;
    ny = az * bx - ax * bz;
    nz = ax * by - ay * bx;
    normalLength = Vec3Length(nx, ny, nz);
    return normalLength > 0.0f && fabsf(ny) >= normalLength * 0.85f;
}

static void LiftRoadDecal(RageNativeDrawVertex triangle[3]) {
    float ax = triangle[1].position[0] - triangle[0].position[0];
    float ay = triangle[1].position[1] - triangle[0].position[1];
    float az = triangle[1].position[2] - triangle[0].position[2];
    float bx = triangle[2].position[0] - triangle[0].position[0];
    float by = triangle[2].position[1] - triangle[0].position[1];
    float bz = triangle[2].position[2] - triangle[0].position[2];
    float nx = ay * bz - az * by;
    float ny = az * bx - ax * bz;
    float nz = ax * by - ay * bx;
    float length = Vec3Length(nx, ny, nz);
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
    RageNativeDrawVertex triangle[3], RageRenderVec3 camera) {
    float ax = triangle[1].position[0] - triangle[0].position[0];
    float ay = triangle[1].position[1] - triangle[0].position[1];
    float az = triangle[1].position[2] - triangle[0].position[2];
    float bx = triangle[2].position[0] - triangle[0].position[0];
    float by = triangle[2].position[1] - triangle[0].position[1];
    float bz = triangle[2].position[2] - triangle[0].position[2];
    float nx = ay * bz - az * by;
    float ny = az * bx - ax * bz;
    float nz = ax * by - ay * bx;
    float cx = (triangle[0].position[0] + triangle[1].position[0] +
                triangle[2].position[0]) / 3.0f;
    float cy = (triangle[0].position[1] + triangle[1].position[1] +
                triangle[2].position[1]) / 3.0f;
    float cz = (triangle[0].position[2] + triangle[1].position[2] +
                triangle[2].position[2]) / 3.0f;
    float length = Vec3Length(nx, ny, nz);
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

static int TriangleIsBackFacing(const RageRenderWorld *world,
                                    const RageNativeDrawVertex triangle[3]) {
    RageRenderVec3 view[3];
    float screenX[3], screenY[3];
    int corner;
    for (corner = 0; corner < 3; corner++) {
        RageRenderVec3 position = {triangle[corner].position[0],
                                   triangle[corner].position[1],
                                   triangle[corner].position[2]};
        float depth;
        RenderWorldToView(&world->camera, &position, &view[corner]);
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
    const RageRenderWorld *world, const RageNativeDrawVertex triangle[3]) {
    RageRenderVec3 input[3], clipped[4];
    uint32_t corner, count, piece;
    for (corner = 0; corner < 3; corner++) {
        RageRenderVec3 position = {triangle[corner].position[0],
                                   triangle[corner].position[1],
                                   triangle[corner].position[2]};
        RenderWorldToView(&world->camera, &position, &input[corner]);
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
    const RageRuntimeMesh *mesh, uint32_t first) {
    RageNativeDrawVertex triangles[2][3] = {0};
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
        if (corner < 3) {
            triangles[0][corner].position[0] = position.x;
            triangles[0][corner].position[1] = position.y;
            triangles[0][corner].position[2] = position.z;
        } else {
            triangles[1][2].position[0] = position.x;
            triangles[1][2].position[1] = position.y;
            triangles[1][2].position[2] = position.z;
        }
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
    return !TerrainTriangleFacesCamera(world, triangles[0]) &&
           !TerrainTriangleFacesCamera(world, triangles[1]);
}

static int InstanceOutsideFrustum(const RageRenderWorld *world,
                                      const RageRenderTransform *transform,
                                      const RageRuntimeMesh *mesh,
                                      uint32_t meshIndex, float aspect) {
    float center[3], radius, maxScale, tanY, tanX, depth;
    float horizontalRadius, verticalRadius;
    RageRenderVec3 worldCenter, view;
    RageTransformBasis basis = BuildTransformBasis(transform);
    if (!RuntimeMeshBounds(mesh, meshIndex, center, &radius)) return 0;
    worldCenter = TransformPoint(&basis, center);
    RenderWorldToView(&world->camera, &worldCenter, &view);
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

static int BuildVertex(const RageTransformBasis *basis,
                           const RageRenderWorld *world, int fogged,
                           const RageRenderMeshInstance *instance,
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
        out->uv[0] += (float)instance->textureScrollU * (1.0f / 256.0f);
        source.material &= ~RAGE_RUNTIME_MATERIAL_SCROLL_U;
    }
    memcpy(out->color, source.color, sizeof(out->color));
    normal = TransformNormal(basis, &source);
    out->normal[0] = normal.x;
    out->normal[1] = normal.y;
    out->normal[2] = normal.z;
    out->fog[0] = world->camera.fogColor.x;
    out->fog[1] = world->camera.fogColor.y;
    out->fog[2] = world->camera.fogColor.z;
    out->fog[3] = fogged
        ? RenderFogFactor(&world->camera, &worldPosition) : 0.0f;
    out->lighting = 0.0f;
    if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_LIGHTING) != 0) {
        out->lighting = instance->lightInfluence;
        if (out->lighting <= 0.0f) out->lighting = 1.0f;
        if (out->lighting > 1.0f) out->lighting = 1.0f;
    }
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
    /* Native scene depth comes only from geometry. PS1 ordering-table hints
     * and old per-instance overlap nudges must never alter the Z buffer. */
    out->depthBias = 0.0f;
    out->shadowReception =
        instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
        instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1
        ? 0.0f : 1.0f;
    *depthDecal =
        (instance->flags & RAGE_RENDER_INSTANCE_DEPTH_DECAL) != 0;
    if ((source.material & RAGE_RUNTIME_MATERIAL_METADATA) != 0) {
        /* The packed byte is a PS1 ordering-table hint, not native material
         * semantics. Strip it with the rest of the import metadata. */
        source.material &= RAGE_RUNTIME_MATERIAL_INDEX_MASK;
        if (source.material == RAGE_RUNTIME_MATERIAL_INDEX_MASK)
            source.material = UINT32_MAX;
    }
    *material = source.material;
    return 1;
}

static uint32_t RenderBuildNativeDrawsFiltered(
    const RageRenderWorld *world, int passFilter, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    uint32_t instanceIndex, vertexCount = 0, spansUsed = 0;
    if (spanCount != NULL) *spanCount = 0;
    if (world == NULL || lookup == NULL || vertices == NULL || spans == NULL ||
        spanCount == NULL || !isfinite(aspect) || aspect <= 0.0f ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCount != 0 && world->instances == NULL)) return 0;
    for (instanceIndex = 0; instanceIndex < world->instanceCount; instanceIndex++) {
        const RageRenderMeshInstance *instance = &world->instances[instanceIndex];
        const RageRuntimeMesh *mesh = lookup(context, instance);
        RageTransformBasis basis;
        uint32_t first, count, offset;
        int terrainQuadHidden = 0;
        if (passFilter >= 0 && instance->pass != (RageRenderPass)passFilter)
            continue;
        if (mesh == NULL || !RuntimeMeshRange(mesh, instance->mesh, &first, &count)) {
            continue;
        }
        if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL) &&
            InstanceOutsideFrustum(world, &instance->transform, mesh,
                                       instance->mesh, aspect)) continue;
        basis = BuildTransformBasis(&instance->transform);
        for (offset = 0; offset + 2 < count; offset += 3) {
            RageNativeDrawVertex triangle[3];
            uint32_t materials[3], materialFlags[3], indices[3];
            uint8_t depthDecals[3];
            uint32_t corner;
            int valid = 1;
            if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN) {
                if ((offset % 6u) == 0)
                    terrainQuadHidden = TerrainQuadIsHidden(
                        world, &basis, mesh, first + offset);
                if (terrainQuadHidden) continue;
            }
            for (corner = 0; corner < 3; corner++) {
                valid = valid && RuntimeMeshIndex(mesh, first + offset + corner,
                                                      &indices[corner]);
                if (valid) valid = BuildVertex(&basis, world,
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
                vertexCount > vertexCapacity ||
                vertexCapacity - vertexCount < 3) continue;
            if ((instance->flags & RAGE_RENDER_INSTANCE_FLAT_SHADED) != 0)
                ApplyFlatTriangleNormal(triangle);
            if (depthDecals[0]) {
                /* Explicit screen/art layers are semantic overlays. Give them
                 * real separation from their backing mesh instead of changing
                 * their depth value in the rasterizer. */
                LiftOverlayTowardCamera(
                    triangle, world->camera.transform.position);
            } else if (instance->assetSet == RAGE_RENDER_ASSET_TERRAIN &&
                materials[0] != UINT32_MAX &&
                TriangleIsRoadDecal(triangle)) {
                LiftRoadDecal(triangle);
                depthDecals[0] = depthDecals[1] = depthDecals[2] = 1;
            }
            if ((instance->flags & RAGE_RENDER_INSTANCE_CULL_BACKFACES) != 0 &&
                TriangleIsBackFacing(world, triangle)) continue;
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
                spans[spansUsed].materialVariant = instance->materialVariant;
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

uint32_t RenderBuildNativeDraws(const RageRenderWorld *world, float aspect,
                                    RageRenderMeshLookup lookup, void *context,
                                    RageNativeDrawVertex *vertices,
                                    uint32_t vertexCapacity,
                                    RageNativeDrawSpan *spans,
                                    uint32_t spanCapacity,
                                    uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        world, -1, aspect, lookup, context, vertices, vertexCapacity, spans,
        spanCapacity, spanCount);
}

uint32_t RenderBuildNativePassDraws(
    const RageRenderWorld *world, RageRenderPass pass, float aspect,
    RageRenderMeshLookup lookup, void *context,
    RageNativeDrawVertex *vertices, uint32_t vertexCapacity,
    RageNativeDrawSpan *spans, uint32_t spanCapacity, uint32_t *spanCount) {
    return RenderBuildNativeDrawsFiltered(
        world, (int)pass, aspect, lookup, context, vertices, vertexCapacity,
        spans, spanCapacity, spanCount);
}
