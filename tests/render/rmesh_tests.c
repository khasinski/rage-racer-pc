#include <stdint.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh.h"

static int failures;

#define EXPECT(value) do { if (!(value)) { failures++; \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__, #value); \
} } while (0)

static void write_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

int main(void) {
    {
        RageRuntimeMeshLayout layout;
        EXPECT(RuntimeMeshLayout(1, 4, 6, &layout));
        EXPECT(layout.offsetsOffset == 24 && layout.verticesOffset == 32);
        EXPECT(layout.indicesOffset == 192 && layout.totalSize == 216);
        EXPECT(RuntimeMeshLayout(0, 0, 0, &layout));
        EXPECT(layout.verticesOffset == 28 && layout.indicesOffset == 28 && layout.totalSize == 28);
        EXPECT(!RuntimeMeshLayout(1, 4, 6, NULL));
        /* No huge allocation: validate the size arithmetic independently. */
        int large = RuntimeMeshLayout(UINT32_MAX, UINT32_MAX, UINT32_MAX, &layout);
        if (SIZE_MAX > UINT32_MAX) {
            EXPECT(large);
            EXPECT((uint64_t)layout.totalSize == UINT64_C(206158430188));
        } else {
            EXPECT(!large);
            EXPECT(layout.totalSize == 0 && layout.offsetsOffset == 0 &&
                   layout.verticesOffset == 0 && layout.indicesOffset == 0);
        }
    }
    uint8_t blob[216] = {0};
    RageRuntimeMesh mesh;
    RageRuntimeVertex vertex;
    uint32_t first, count, index;
    float center[3];
    float radius;
    float position[3] = {3.0f, 4.0f, 5.0f};
    float normal[3] = {0.0f, 1.0f, 0.0f};
    float uv[2] = {0.5f, 0.25f};
    uint32_t indices[6] = {0, 2, 1, 1, 2, 3};

    memcpy(blob, "RRMESH1", 7);
    write_u32(blob + 8, 1);
    write_u32(blob + 12, 1);
    write_u32(blob + 16, 4);
    write_u32(blob + 20, 6);
    {
        uint8_t encoded[sizeof(blob)];
        memset(encoded, 0xA5, sizeof(encoded));
        EXPECT(!RuntimeMeshEncodeHeader(NULL, sizeof(encoded), 1, 4, 6));
        EXPECT(!RuntimeMeshEncodeHeader(encoded, sizeof(encoded) - 1, 1, 4, 6));
        for (size_t i = 0; i < sizeof(encoded); ++i) EXPECT(encoded[i] == 0xA5);
        EXPECT(RuntimeMeshEncodeHeader(encoded, sizeof(encoded), 1, 4, 6));
        EXPECT(memcmp(encoded, blob, 24) == 0);
        for (size_t i = 24; i < sizeof(encoded); ++i) EXPECT(encoded[i] == 0xA5);
        /* Header success does not bless an uninitialized payload. */
        RageRuntimeMesh invalid;
        EXPECT(!RuntimeMeshOpen(&invalid, encoded, sizeof(encoded)));
        uint8_t empty[28] = {0};
        EXPECT(RuntimeMeshEncodeHeader(empty, sizeof(empty), 0, 0, 0));
        EXPECT(RuntimeMeshOpen(&invalid, empty, sizeof(empty)));
        EXPECT(invalid.meshCount == 0 && invalid.vertexCount == 0 && invalid.indexCount == 0);
    }
    write_u32(blob + 24, 0);
    write_u32(blob + 28, 6);
    memcpy(blob + 32, position, sizeof(position));
    memcpy(blob + 44, normal, sizeof(normal));
    blob[56] = 1; blob[57] = 2; blob[58] = 3; blob[59] = 255;
    memcpy(blob + 60, uv, sizeof(uv));
    write_u32(blob + 68, 7);
    memcpy(blob + 192, indices, sizeof(indices));

    EXPECT(RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    EXPECT(RuntimeMeshRange(&mesh, 0, &first, &count));
    EXPECT(first == 0 && count == 6);
    EXPECT(RuntimeMeshVertex(&mesh, 0, &vertex));
    EXPECT((int)vertex.position[0] == 3 && vertex.color[2] == 3);
    EXPECT((int)(vertex.uv[0] * 100.0f) == 50 && vertex.material == 7);
    EXPECT(RuntimeMeshIndex(&mesh, 1, &index) && index == 2);
    {
        uint8_t encoded[RAGE_RUNTIME_VERTEX_BYTES + 1];
        memset(encoded, 0xA5, sizeof(encoded));
        EXPECT(RuntimeVertexEncode(encoded, sizeof(encoded), &vertex));
        EXPECT(memcmp(encoded, blob + 32, RAGE_RUNTIME_VERTEX_BYTES) == 0);
        EXPECT(encoded[RAGE_RUNTIME_VERTEX_BYTES] == 0xA5);
        /* Independent wire bytes for 3.0f, 0.5f and the material number. */
        EXPECT(encoded[0] == 0 && encoded[1] == 0 && encoded[2] == 0x40 && encoded[3] == 0x40);
        EXPECT(encoded[28] == 0 && encoded[29] == 0 && encoded[30] == 0 && encoded[31] == 0x3F);
        EXPECT(encoded[36] == 7 && encoded[37] == 0 && encoded[38] == 0 && encoded[39] == 0);
        uint8_t saved[sizeof(encoded)];
        memcpy(saved, encoded, sizeof(saved));
        EXPECT(!RuntimeVertexEncode(encoded, RAGE_RUNTIME_VERTEX_BYTES - 1, &vertex));
        EXPECT(!RuntimeVertexEncode(encoded, sizeof(encoded), NULL));
        EXPECT(!RuntimeVertexEncode(NULL, sizeof(encoded), &vertex));
        for (unsigned i = 0; i < 8; ++i) {
            RageRuntimeVertex invalid = vertex;
            float *field = i < 3 ? &invalid.position[i] :
                i < 6 ? &invalid.normal[i - 3] : &invalid.uv[i - 6];
            *field = i % 2 ? INFINITY : NAN;
            EXPECT(!RuntimeVertexEncode(encoded, sizeof(encoded), &invalid));
            EXPECT(memcmp(encoded, saved, sizeof(encoded)) == 0);
        }
        vertex.material = UINT32_MAX;
        EXPECT(RuntimeVertexEncode(encoded, sizeof(encoded), &vertex));
        EXPECT(encoded[36] == 255 && encoded[37] == 255 && encoded[38] == 255 && encoded[39] == 255);
        const uint32_t materials[] = {
            0, UINT32_MAX, RAGE_RUNTIME_MATERIAL_INDEX_MASK,
            RAGE_RUNTIME_MATERIAL_SCROLL_U | 7,
            RAGE_RUNTIME_MATERIAL_METADATA | RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY | 9,
            RAGE_RUNTIME_MATERIAL_METADATA | RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT | 11,
            RAGE_RUNTIME_MATERIAL_METADATA | (255u << RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT) | 13
        };
        uint8_t roundTrip[sizeof(blob)];
        memcpy(roundTrip, blob, sizeof(blob));
        for (size_t i = 0; i < sizeof(materials) / sizeof(materials[0]); ++i) {
            RageRuntimeMesh decodedMesh;
            RageRuntimeVertex decoded;
            vertex.material = materials[i];
            EXPECT(RuntimeVertexEncode(roundTrip + 32, RAGE_RUNTIME_VERTEX_BYTES, &vertex));
            EXPECT(RuntimeMeshOpen(&decodedMesh, roundTrip, sizeof(roundTrip)));
            EXPECT(RuntimeMeshVertex(&decodedMesh, 0, &decoded));
            EXPECT(decoded.material == materials[i]);
            EXPECT(memcmp(decoded.position, vertex.position, sizeof(vertex.position)) == 0);
            EXPECT(memcmp(decoded.normal, vertex.normal, sizeof(vertex.normal)) == 0);
            EXPECT(memcmp(decoded.color, vertex.color, sizeof(vertex.color)) == 0);
            EXPECT(memcmp(decoded.uv, vertex.uv, sizeof(vertex.uv)) == 0);
        }
    }
    {
        RageRuntimeMeshBounds bounds[1];
        float expectedCenter[3], expectedRadius;
        EXPECT(RuntimeMeshBounds(&mesh, 0, expectedCenter, &expectedRadius));
        EXPECT(!RuntimeMeshPrepareBounds(&mesh, bounds, 0));
        EXPECT(mesh.bounds == NULL);
        EXPECT(RuntimeMeshPrepareBounds(&mesh, bounds, 1));
        EXPECT(mesh.bounds == bounds);
        EXPECT(RuntimeMeshBounds(&mesh, 0, center, &radius));
        EXPECT(memcmp(center, expectedCenter, sizeof(center)) == 0);
        EXPECT(radius == expectedRadius);
        EXPECT(!RuntimeMeshBounds(&mesh, 1, center, &radius));
        EXPECT(center[0] == 0.0f && radius == 0.0f);
        EXPECT(!RuntimeMeshPrepareBounds(&mesh, NULL, 1));
        EXPECT(RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
        EXPECT(mesh.bounds == NULL);
    }
    blob[0] = 0;
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    EXPECT(mesh.bytes == NULL && mesh.meshCount == 0);
    blob[0] = 'R';
    write_u32(blob + 28, 7);
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    write_u32(blob + 28, 6);
    write_u32(blob + 192, 4);
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    write_u32(blob + 192, 0);

    write_u32(blob + 20, 5);
    write_u32(blob + 28, 5);
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    write_u32(blob + 20, 6);
    write_u32(blob + 28, 6);

    position[0] = NAN;
    memcpy(blob + 32, position, sizeof(position));
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    position[0] = 3.0f;
    memcpy(blob + 32, position, sizeof(position));
    uv[1] = INFINITY;
    memcpy(blob + 60, uv, sizeof(uv));
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));

    memset(&mesh, 0, sizeof(mesh));
    first = count = index = 123;
    memset(&vertex, 0x7f, sizeof(vertex));
    center[0] = center[1] = center[2] = radius = 123.0f;
    EXPECT(!RuntimeMeshRange(&mesh, 0, &first, &count));
    EXPECT(first == 0 && count == 0);
    EXPECT(!RuntimeMeshVertex(&mesh, 0, &vertex));
    EXPECT(vertex.position[0] == 0.0f && vertex.material == 0);
    EXPECT(!RuntimeMeshIndex(&mesh, 0, &index) && index == 0);
    EXPECT(!RuntimeMeshBounds(&mesh, 0, center, &radius));
    EXPECT(center[0] == 0.0f && center[1] == 0.0f &&
           center[2] == 0.0f && radius == 0.0f);

    mesh.bytes = blob;
    mesh.size = sizeof(blob);
    mesh.meshCount = mesh.vertexCount = mesh.indexCount = 1;
    mesh.offsetsOffset = mesh.verticesOffset = mesh.indicesOffset = SIZE_MAX;
    first = count = index = 123;
    memset(&vertex, 0x7f, sizeof(vertex));
    EXPECT(!RuntimeMeshRange(&mesh, 0, &first, &count));
    EXPECT(first == 0 && count == 0);
    EXPECT(!RuntimeMeshVertex(&mesh, 0, &vertex));
    EXPECT(vertex.position[0] == 0.0f && vertex.material == 0);
    EXPECT(!RuntimeMeshIndex(&mesh, 0, &index) && index == 0);

    uv[1] = 0.25f;
    memcpy(blob + 60, uv, sizeof(uv));
    position[0] = FLT_MAX;
    position[1] = FLT_MAX;
    memcpy(blob + 32, position, sizeof(position));
    position[0] = -FLT_MAX;
    position[1] = -FLT_MAX;
    memcpy(blob + 72, position, sizeof(position));
    EXPECT(RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
    center[0] = center[1] = center[2] = radius = 123.0f;
    EXPECT(!RuntimeMeshBounds(&mesh, 0, center, &radius));
    EXPECT(center[0] == 0.0f && radius == 0.0f);
    {
        RageRuntimeMeshBounds bounds[1];
        EXPECT(RuntimeMeshPrepareBounds(&mesh, bounds, 1));
        EXPECT(!bounds[0].valid);
        EXPECT(!RuntimeMeshBounds(&mesh, 0, center, &radius));
        EXPECT(center[0] == 0.0f && radius == 0.0f);
        EXPECT(!RuntimeMeshOpen(&mesh, NULL, 0));
        EXPECT(mesh.bounds == NULL);
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
