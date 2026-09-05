#include "rmesh.h"

#include <float.h>
#include <math.h>
#include <string.h>

enum {
    RAGE_RMESH_HEADER_SIZE = 24,
    RAGE_RMESH_VERTEX_SIZE = RAGE_RUNTIME_VERTEX_BYTES,
};

static const uint8_t s_magic[8] = {'R', 'R', 'M', 'E', 'S', 'H', '1', 0};

static uint32_t RageReadU32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static float RageReadFloat(const uint8_t *p) {
    uint32_t bits = RageReadU32(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void RageWriteU32(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (8 * i));
}

static void RageWriteFloat(uint8_t *p, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    RageWriteU32(p, bits);
}

int RuntimeVertexEncode(void *bytes, size_t size, const RageRuntimeVertex *vertex) {
    uint8_t encoded[RAGE_RUNTIME_VERTEX_BYTES];
    if (bytes == NULL || vertex == NULL || size < sizeof(encoded)) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (!isfinite(vertex->position[i]) || !isfinite(vertex->normal[i])) return 0;
        RageWriteFloat(encoded + i * 4, vertex->position[i]);
        RageWriteFloat(encoded + 12 + i * 4, vertex->normal[i]);
    }
    for (unsigned i = 0; i < 2; ++i) {
        if (!isfinite(vertex->uv[i])) return 0;
        RageWriteFloat(encoded + 28 + i * 4, vertex->uv[i]);
    }
    memcpy(encoded + 24, vertex->color, 4);
    RageWriteU32(encoded + 36, vertex->material);
    memcpy(bytes, encoded, sizeof(encoded));
    return 1;
}

static int RageFloatArrayIsFinite(const uint8_t *bytes, size_t count) {
    size_t index;
    for (index = 0; index < count; index++) {
        float value = RageReadFloat(bytes + index * sizeof(float));
        if (!isfinite(value)) return 0;
    }
    return 1;
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

static int ElementRange(size_t base, size_t index, size_t elementSize,
                        size_t bufferSize, size_t *offset) {
    size_t relative;
    return SizeMultiply(index, elementSize, &relative) &&
           SizeAdd(base, relative, offset) &&
           Range(*offset, elementSize, bufferSize);
}

int RuntimeMeshLayout(uint32_t meshes, uint32_t vertices, uint32_t indices,
                      RageRuntimeMeshLayout *out) {
    RageRuntimeMeshLayout layout = {0};
    size_t count, offsetsBytes, verticesBytes, indicesBytes;
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    if (!SizeAdd(meshes, 1, &count) ||
        !SizeMultiply(count, 4, &offsetsBytes) ||
        !SizeMultiply(vertices, RAGE_RUNTIME_VERTEX_BYTES, &verticesBytes) ||
        !SizeMultiply(indices, 4, &indicesBytes) ||
        !SizeAdd(RAGE_RMESH_HEADER_SIZE, offsetsBytes, &layout.verticesOffset) ||
        !SizeAdd(layout.verticesOffset, verticesBytes, &layout.indicesOffset) ||
        !SizeAdd(layout.indicesOffset, indicesBytes, &layout.totalSize)) return 0;
    layout.offsetsOffset = RAGE_RMESH_HEADER_SIZE;
    *out = layout;
    return 1;
}

int RuntimeMeshEncodeHeader(void *bytes, size_t size, uint32_t meshes,
                            uint32_t vertices, uint32_t indices) {
    RageRuntimeMeshLayout layout;
    uint8_t *p = bytes;
    if (p == NULL || !RuntimeMeshLayout(meshes, vertices, indices, &layout) ||
        size < layout.totalSize) return 0;
    memcpy(p, s_magic, sizeof(s_magic));
    RageWriteU32(p + 8, 1);
    RageWriteU32(p + 12, meshes);
    RageWriteU32(p + 16, vertices);
    RageWriteU32(p + 20, indices);
    return 1;
}

int RuntimeMeshOpen(RageRuntimeMesh *mesh, const void *bytes, size_t size) {
    const uint8_t *p = bytes;
    uint32_t version, meshCount, vertexCount, indexCount;
    RageRuntimeMeshLayout layout;
    size_t verticesOffset, indicesOffset;
    uint32_t previous;
    uint32_t i;

    if (mesh == NULL) return 0;
    memset(mesh, 0, sizeof(*mesh));
    if (p == NULL || size < RAGE_RMESH_HEADER_SIZE ||
        memcmp(p, s_magic, sizeof(s_magic)) != 0) return 0;
    version = RageReadU32(p + 8);
    meshCount = RageReadU32(p + 12);
    vertexCount = RageReadU32(p + 16);
    indexCount = RageReadU32(p + 20);
    if (version != 1 || !RuntimeMeshLayout(meshCount, vertexCount, indexCount, &layout) ||
        layout.totalSize > size) return 0;
    verticesOffset = layout.verticesOffset;
    indicesOffset = layout.indicesOffset;
    previous = RageReadU32(p + RAGE_RMESH_HEADER_SIZE);
    if (previous != 0) return 0;
    for (size_t offsetIndex = 1; offsetIndex <= (size_t)meshCount; ++offsetIndex) {
        uint32_t offset = RageReadU32(p + layout.offsetsOffset + offsetIndex * 4);
        if (offset < previous || offset > indexCount ||
            (offset - previous) % 3u != 0) return 0;
        previous = offset;
    }
    if (previous != indexCount) return 0;
    for (i = 0; i < vertexCount; i++) {
        const uint8_t *vertex = p + verticesOffset +
            (size_t)i * RAGE_RMESH_VERTEX_SIZE;
        if (!RageFloatArrayIsFinite(vertex, 6) ||
            !RageFloatArrayIsFinite(vertex + 28, 2)) return 0;
    }
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
    size_t firstOffset, endOffset;
    if (firstIndex == NULL || indexCount == NULL) return 0;
    *firstIndex = 0;
    *indexCount = 0;
    if (mesh == NULL || mesh->bytes == NULL ||
        meshIndex >= mesh->meshCount ||
        !ElementRange(mesh->offsetsOffset, meshIndex, sizeof(uint32_t),
                      mesh->size, &firstOffset) ||
        !ElementRange(mesh->offsetsOffset, (size_t)meshIndex + 1,
                      sizeof(uint32_t), mesh->size, &endOffset)) return 0;
    first = RageReadU32(mesh->bytes + firstOffset);
    end = RageReadU32(mesh->bytes + endOffset);
    if (first > end || end > mesh->indexCount) return 0;
    *firstIndex = first;
    *indexCount = end - first;
    return 1;
}

int RuntimeMeshVertex(const RageRuntimeMesh *mesh, uint32_t vertexIndex,
                          RageRuntimeVertex *out) {
    const uint8_t *p;
    size_t offset;
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    if (mesh == NULL || mesh->bytes == NULL ||
        vertexIndex >= mesh->vertexCount ||
        !ElementRange(mesh->verticesOffset, vertexIndex,
                      RAGE_RMESH_VERTEX_SIZE, mesh->size, &offset)) return 0;
    p = mesh->bytes + offset;
    for (unsigned i = 0; i < 3; ++i) {
        out->position[i] = RageReadFloat(p + i * 4);
        out->normal[i] = RageReadFloat(p + 12 + i * 4);
    }
    memcpy(out->color, p + 24, sizeof(out->color));
    for (unsigned i = 0; i < 2; ++i) out->uv[i] = RageReadFloat(p + 28 + i * 4);
    out->material = RageReadU32(p + 36);
    return 1;
}

int RuntimeMeshIndex(const RageRuntimeMesh *mesh, uint32_t indexIndex,
                         uint32_t *out) {
    uint32_t index;
    size_t offset;
    if (out == NULL) return 0;
    *out = 0;
    if (mesh == NULL || mesh->bytes == NULL ||
        indexIndex >= mesh->indexCount ||
        !ElementRange(mesh->indicesOffset, indexIndex, sizeof(uint32_t),
                      mesh->size, &offset)) return 0;
    index = RageReadU32(mesh->bytes + offset);
    if (index >= mesh->vertexCount) return 0;
    *out = index;
    return 1;
}

int RuntimeMeshBounds(const RageRuntimeMesh *mesh, uint32_t meshIndex,
                          float center[3], float *radius) {
    RageRuntimeVertex vertex;
    uint32_t first, count, offset, index;
    float min[3], max[3];
    float resultCenter[3];
    double radiusSquared = 0.0;

    if (center == NULL || radius == NULL) return 0;
    memset(center, 0, sizeof(resultCenter));
    *radius = 0.0f;
    if (mesh != NULL && mesh->bounds != NULL && meshIndex < mesh->meshCount) {
        const RageRuntimeMeshBounds *cached = &mesh->bounds[meshIndex];
        if (!cached->valid) return 0;
        memcpy(center, cached->center, sizeof(cached->center));
        *radius = cached->radius;
        return 1;
    }
    if (!RuntimeMeshRange(mesh, meshIndex, &first, &count) || count == 0)
        return 0;
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
    for (offset = 0; offset < 3; offset++) {
        double middle = ((double)min[offset] + (double)max[offset]) * 0.5;
        double extent = (double)max[offset] - middle;

        if (!isfinite(middle) || middle < -FLT_MAX || middle > FLT_MAX)
            return 0;
        resultCenter[offset] = (float)middle;
        radiusSquared += extent * extent;
    }
    if (!isfinite(radiusSquared) || radiusSquared > (double)FLT_MAX * FLT_MAX)
        return 0;
    memcpy(center, resultCenter, sizeof(resultCenter));
    *radius = (float)sqrt(radiusSquared);
    return 1;
}

int RuntimeMeshPrepareBounds(RageRuntimeMesh *mesh,
                            RageRuntimeMeshBounds *storage, size_t capacity) {
    uint32_t i;
    if (mesh == NULL || storage == NULL || capacity < mesh->meshCount) return 0;
    mesh->bounds = NULL;
    for (i = 0; i < mesh->meshCount; ++i)
        storage[i].valid = RuntimeMeshBounds(mesh, i, storage[i].center,
                                            &storage[i].radius);
    mesh->bounds = storage;
    return 1;
}
