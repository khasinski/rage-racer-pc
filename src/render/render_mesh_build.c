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

static int RageInstanceOutsideFrustum(const RageRenderWorld *world,
                                      const RageRenderTransform *transform,
                                      const RageRuntimeMesh *mesh,
                                      uint32_t meshIndex, float aspect) {
    float center[3], radius, maxScale, halfY, halfX;
    RageRenderVec3 worldCenter, view;
    RageTransformBasis basis = RageBuildTransformBasis(transform);
    if (!RageRuntimeMeshBounds(mesh, meshIndex, center, &radius)) return 0;
    worldCenter = RageTransformPoint(&basis, center);
    RageRenderWorldToView(&world->camera, &worldCenter, &view);
    maxScale = fmaxf(fabsf(transform->scale.x),
                     fmaxf(fabsf(transform->scale.y), fabsf(transform->scale.z)));
    radius *= maxScale;
    if (view.z + radius < world->camera.nearPlane ||
        view.z - radius > world->camera.farPlane) return 1;
    halfY = fmaxf(view.z, world->camera.nearPlane) *
        tanf(RageRadians(world->camera.verticalFovDegrees) * 0.5f);
    halfX = halfY * aspect;
    return view.x + radius < -halfX || view.x - radius > halfX ||
           view.y + radius < -halfY || view.y - radius > halfY;
}

static int RageBuildVertex(const RageTransformBasis *basis,
                           const RageRuntimeMesh *mesh, uint32_t index,
                           float aspect, RageNativeDrawVertex *out,
                           uint32_t *material) {
    RageRuntimeVertex source;
    RageRenderVec3 world;
    if (!RageRuntimeMeshVertex(mesh, index, &source)) return 0;
    world = RageTransformPosition(basis, &source);
    (void)aspect;
    out->position[0] = world.x; out->position[1] = world.y;
    out->position[2] = world.z;
    out->uv[0] = source.uv[0]; out->uv[1] = source.uv[1];
    memcpy(out->color, source.color, sizeof(out->color));
    {
        RageRenderVec3 normal = RageTransformNormal(basis, &source);
        out->normal[0] = normal.x; out->normal[1] = normal.y;
        out->normal[2] = normal.z;
    }
    *material = source.material;
    return 1;
}

uint32_t RageRenderBuildNativeDraws(const RageRenderWorld *world, float aspect,
                                    RageRenderMeshLookup lookup, void *context,
                                    RageNativeDrawVertex *vertices,
                                    uint32_t vertexCapacity,
                                    RageNativeDrawSpan *spans,
                                    uint32_t spanCapacity,
                                    uint32_t *spanCount) {
    uint32_t instanceIndex, vertexCount = 0, spansUsed = 0;
    if (spanCount != NULL) *spanCount = 0;
    if (world == NULL || lookup == NULL || vertices == NULL || spans == NULL ||
        spanCount == NULL) return 0;
    for (instanceIndex = 0; instanceIndex < world->instanceCount; instanceIndex++) {
        const RageRenderMeshInstance *instance = &world->instances[instanceIndex];
        const RageRuntimeMesh *mesh = lookup(context, instance);
        RageTransformBasis basis;
        uint32_t first, count, offset;
        if (mesh == NULL || !RageRuntimeMeshRange(mesh, instance->mesh, &first, &count)) {
            continue;
        }
        if ((instance->flags & RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL) &&
            RageInstanceOutsideFrustum(world, &instance->transform, mesh,
                                       instance->mesh, aspect)) continue;
        basis = RageBuildTransformBasis(&instance->transform);
        for (offset = 0; offset + 2 < count; offset += 3) {
            RageNativeDrawVertex triangle[3];
            uint32_t materials[3], indices[3];
            uint32_t corner;
            int valid = 1;
            for (corner = 0; corner < 3; corner++) {
                valid = valid && RageRuntimeMeshIndex(mesh, first + offset + corner,
                                                      &indices[corner]);
                if (valid) valid = RageBuildVertex(&basis,
                    mesh, indices[corner], aspect,
                    &triangle[corner], &materials[corner]);
            }
            if (!valid ||
                materials[0] != materials[1] || materials[0] != materials[2] ||
                vertexCount + 3 > vertexCapacity) continue;
            if (spansUsed == 0 || spans[spansUsed - 1].material != materials[0] ||
                spans[spansUsed - 1].assetKey != instance->assetKey ||
                spans[spansUsed - 1].assetSet != instance->assetSet ||
                spans[spansUsed - 1].entity !=
                    (instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK
                     ? instance->entity : 0) ||
                spans[spansUsed - 1].pass != instance->pass) {
                if (spansUsed == spanCapacity) goto done;
                spans[spansUsed].firstVertex = vertexCount;
                spans[spansUsed].vertexCount = 0;
                spans[spansUsed].assetKey = instance->assetKey;
                spans[spansUsed].assetSet = instance->assetSet;
                spans[spansUsed].material = materials[0];
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

