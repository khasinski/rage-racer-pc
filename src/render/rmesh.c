#include "rmesh.h"

#include <math.h>
#include <string.h>

enum {
    RAGE_RMESH_HEADER_SIZE = 24,
    RAGE_RMESH_VERTEX_SIZE = 40,
};

static const uint8_t s_magic[8] = {'R', 'R', 'M', 'E', 'S', 'H', '1', 0};

static uint32_t RageReadU32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int Range(size_t offset, size_t length, size_t size) {
    return offset <= size && length <= size - offset;
}

static int SizeAdd(size_t left, size_t right, size_t *result) {
    if (right > SIZE_MAX - left) return 0;
    *result = left + right;
    return 1;
}

static int SizeMultiply(size_t left, size_t right, size_t *result) {
    if (right != 0 && left > SIZE_MAX / right) return 0;
    *result = left * right;
    return 1;
}

int RuntimeMeshOpen(RageRuntimeMesh *mesh, const void *bytes, size_t size) {
    const uint8_t *p = bytes;
    uint32_t version, meshCount, vertexCount, indexCount;
    size_t offsetCount, offsetsBytes, verticesBytes, indicesBytes;
    size_t verticesOffset, indicesOffset;
    uint32_t previous;
    uint32_t i;

    if (mesh == 0 || p == 0 || size < RAGE_RMESH_HEADER_SIZE ||
        memcmp(p, s_magic, sizeof(s_magic)) != 0) return 0;
    version = RageReadU32(p + 8);
    meshCount = RageReadU32(p + 12);
    vertexCount = RageReadU32(p + 16);
    indexCount = RageReadU32(p + 20);
    if (version != 1 ||
        !SizeAdd((size_t)meshCount, 1, &offsetCount) ||
        !SizeMultiply(offsetCount, 4, &offsetsBytes) ||
        !SizeMultiply((size_t)vertexCount, RAGE_RMESH_VERTEX_SIZE,
                          &verticesBytes) ||
        !SizeMultiply((size_t)indexCount, 4, &indicesBytes) ||
        !SizeAdd(RAGE_RMESH_HEADER_SIZE, offsetsBytes, &verticesOffset) ||
        !SizeAdd(verticesOffset, verticesBytes, &indicesOffset)) return 0;
    if (!Range(RAGE_RMESH_HEADER_SIZE, offsetsBytes, size) ||
        !Range(verticesOffset, verticesBytes, size) ||
        !Range(indicesOffset, indicesBytes, size)) return 0;
    previous = RageReadU32(p + RAGE_RMESH_HEADER_SIZE);
    if (previous != 0) return 0;
    for (i = 1; i <= meshCount; i++) {
        uint32_t offset = RageReadU32(p + RAGE_RMESH_HEADER_SIZE + i * 4);
        if (offset < previous || offset > indexCount) return 0;
        previous = offset;
    }
    if (previous != indexCount) return 0;
    for (i = 0; i < indexCount; i++) {
        if (RageReadU32(p + indicesOffset + (size_t)i * 4) >= vertexCount) {
            return 0;
        }
    }
    mesh->bytes = p;
    mesh->size = size;
    mesh->meshCount = meshCount;
    mesh->vertexCount = vertexCount;
    mesh->indexCount = indexCount;
    mesh->offsetsOffset = RAGE_RMESH_HEADER_SIZE;
    mesh->verticesOffset = verticesOffset;
    mesh->indicesOffset = indicesOffset;
    return 1;
}

int RuntimeMeshRange(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                         uint32_t *firstIndex, uint32_t *indexCount) {
    uint32_t first, end;
    if (mesh == 0 || firstIndex == 0 || indexCount == 0 ||
        meshIndex >= mesh->meshCount) return 0;
    first = RageReadU32(mesh->bytes + mesh->offsetsOffset + meshIndex * 4);
    end = RageReadU32(mesh->bytes + mesh->offsetsOffset + (meshIndex + 1) * 4);
    if (first > end || end > mesh->indexCount) return 0;
    *firstIndex = first;
    *indexCount = end - first;
    return 1;
}

int RuntimeMeshVertex(const RageRuntimeMesh *mesh, uint32_t vertexIndex,
                          RageRuntimeVertex *out) {
    const uint8_t *p;
    if (mesh == 0 || out == 0 || vertexIndex >= mesh->vertexCount) return 0;
    p = mesh->bytes + mesh->verticesOffset + (size_t)vertexIndex * RAGE_RMESH_VERTEX_SIZE;
    memcpy(out->position, p, sizeof(out->position));
    memcpy(out->normal, p + 12, sizeof(out->normal));
    memcpy(out->color, p + 24, sizeof(out->color));
    memcpy(out->uv, p + 28, sizeof(out->uv));
    out->material = RageReadU32(p + 36);
    return 1;
}

int RuntimeMeshIndex(const RageRuntimeMesh *mesh, uint32_t indexIndex,
                         uint32_t *out) {
    uint32_t index;
    if (mesh == 0 || out == 0 || indexIndex >= mesh->indexCount) return 0;
    index = RageReadU32(mesh->bytes + mesh->indicesOffset + indexIndex * 4);
    if (index >= mesh->vertexCount) return 0;
    *out = index;
    return 1;
}

int RuntimeMeshBounds(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                          float center[3], float *radius) {
    RageRuntimeVertex vertex;
    uint32_t first, count, offset, index;
    float min[3], max[3];
    if (center == 0 || radius == 0 || !RuntimeMeshRange(mesh, meshIndex,
        &first, &count) || count == 0) return 0;
    if (!RuntimeMeshIndex(mesh, first, &index) ||
        !RuntimeMeshVertex(mesh, index, &vertex)) return 0;
    memcpy(min, vertex.position, sizeof(min));
    memcpy(max, vertex.position, sizeof(max));
    for (offset = 1; offset < count; offset++) {
        uint32_t axis;
        if (!RuntimeMeshIndex(mesh, first + offset, &index) ||
            !RuntimeMeshVertex(mesh, index, &vertex)) return 0;
        for (axis = 0; axis < 3; axis++) {
            if (vertex.position[axis] < min[axis]) min[axis] = vertex.position[axis];
            if (vertex.position[axis] > max[axis]) max[axis] = vertex.position[axis];
        }
    }
    for (offset = 0; offset < 3; offset++) center[offset] = (min[offset] + max[offset]) * 0.5f;
    *radius = sqrtf((max[0] - center[0]) * (max[0] - center[0]) +
                    (max[1] - center[1]) * (max[1] - center[1]) +
                    (max[2] - center[2]) * (max[2] - center[2]));
    return 1;
}
