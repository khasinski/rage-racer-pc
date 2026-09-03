#include <stdint.h>
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
    uint8_t blob[216] = {0};
    RageRuntimeMesh mesh;
    RageRuntimeVertex vertex;
    uint32_t first, count, index;
    float position[3] = {3.0f, 4.0f, 5.0f};
    float normal[3] = {0.0f, 1.0f, 0.0f};
    float uv[2] = {0.5f, 0.25f};
    uint32_t indices[6] = {0, 2, 1, 1, 2, 3};

    memcpy(blob, "RRMESH1", 7);
    write_u32(blob + 8, 1);
    write_u32(blob + 12, 1);
    write_u32(blob + 16, 4);
    write_u32(blob + 20, 6);
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
    blob[0] = 0;
    EXPECT(!RuntimeMeshOpen(&mesh, blob, sizeof(blob)));
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
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
