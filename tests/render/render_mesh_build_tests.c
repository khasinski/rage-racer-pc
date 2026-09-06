#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/render_mesh_build.h"
#include "render/render_native_vertex.h"
#include "render/authored_car_surface.h"

static int failures;

static void write_u32(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static const RageRuntimeMesh *test_mesh_lookup(
    void *context, const RageRenderMeshInstance *instance) {
    (void)instance;
    return context;
}

#define EXPECT_EQ(expected, actual) do {                                      \
    unsigned long long expected_value = (unsigned long long)(expected);       \
    unsigned long long actual_value = (unsigned long long)(actual);           \
    if (expected_value != actual_value) {                                     \
        fprintf(stderr, "%s:%d: expected %llu, got %llu\\n", __FILE__,      \
                __LINE__, expected_value, actual_value);                      \
        failures++;                                                            \
    }                                                                          \
} while (0)

#define EXPECT_NEAR(expected, actual, tolerance) do {                         \
    float expected_value = (float)(expected);                                 \
    float actual_value = (float)(actual);                                     \
    if (fabsf(expected_value - actual_value) > (float)(tolerance)) {           \
        fprintf(stderr, "%s:%d: expected %.4f, got %.4f\\n", __FILE__,      \
                __LINE__, (double)expected_value, (double)actual_value);       \
        failures++;                                                           \
    }                                                                         \
} while (0)

static void test_native_draw_builder_uses_render_world_and_imported_mesh(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[2] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1.0f, 0.0f, 10.0f},
                             {1.0f, 0.0f, 10.0f},
                             {0.0f, 1.0f, 10.0f}};
    unsigned i;
    uint32_t spanCount;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        {
            float normal[3] = {0.0f, 1.0f, 0.0f};
            memcpy(bytes + 32 + i * 40 + 12, normal, sizeof(normal));
        }
        bytes[32 + i * 40 + 24] = 100;
        bytes[32 + i * 40 + 25] = 150;
        bytes[32 + i * 40 + 26] = 200;
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 32 + i * 40 + 36, 4);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 2);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f;
    world.camera.farPlane = 100.0f;
    world.camera.fogColor.x = 0.25f;
    world.camera.fogColor.y = 0.5f;
    world.camera.fogColor.z = 0.75f;
    storage[0].mesh = 0;
    storage[0].assetKey = 10;
    storage[0].pass = RAGE_RENDER_PASS_MAIN;
    storage[0].materialVariant = 1;
    storage[0].hasCarPaint = 1;
    storage[0].carPaintColor1 = 3;
    storage[0].carPaintColor2 = 12;
    storage[0].component = 3;
    storage[0].environmentLight.x = 0.25f;
    storage[0].environmentLight.y = 0.5f;
    storage[0].environmentLight.z = 0.75f;
    storage[0].depthBias = -16.0f;
    storage[0].transform.scale.x = 1.0f;
    storage[0].transform.scale.y = 1.0f;
    storage[0].transform.scale.z = 1.0f;
    /* Ordinary Euler transforms remain a supported scene representation;
     * they must never accidentally consume a stale quaternion basis. */
    storage[0].transform.rotation.y = 90.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(10, spans[0].assetKey);
    EXPECT_EQ(RAGE_RENDER_ASSET_MODEL_BANK, spans[0].assetSet);
    EXPECT_EQ(0, spans[0].depthDecal);
    EXPECT_EQ(1, spans[0].materialVariant);
    EXPECT_EQ(1, spans[0].hasCarPaint);
    EXPECT_EQ(3, spans[0].carPaintColor1);
    EXPECT_EQ(12, spans[0].carPaintColor2);
    EXPECT_EQ(3, spans[0].component);
    /* Per-instance basis matches the old X/Y/Z rotation order without
     * recalculating trigonometry for every emitted vertex. */
    EXPECT_NEAR(10.0f, vertices[0].position[0], 0.001f);
    EXPECT_NEAR(1.0f, vertices[0].position[2], 0.001f);
    EXPECT_EQ(200, vertices[0].color[2]);
    EXPECT_EQ(100, (int)(vertices[0].normal[1] * 100.0f));
    EXPECT_EQ(25, (int)(vertices[0].fog[0] * 100.0f));
    EXPECT_EQ(50, (int)(vertices[0].fog[1] * 100.0f));
    EXPECT_EQ(75, (int)(vertices[0].fog[2] * 100.0f));
    EXPECT_EQ(0, (int)(vertices[0].fog[3] * 100.0f));
    EXPECT_EQ(0, (int)(vertices[0].lighting * 100.0f));

    /* Quaternion normalization must not overflow for a perfectly valid
     * orientation whose components happen to use a large common scale. */
    storage[0].transform.rotation.y = 0.0f;
    storage[0].transform.hasOrientation = 1;
    storage[0].transform.orientation.y = FLT_MAX;
    storage[0].transform.orientation.w = FLT_MAX;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_NEAR(10.0f, vertices[0].position[0], 0.001f);
    EXPECT_NEAR(1.0f, vertices[0].position[2], 0.001f);
    storage[0].transform.hasOrientation = 0;
    storage[0].transform.rotation.y = 90.0f;

    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(100, (int)(vertices[0].lighting * 100.0f));
    EXPECT_EQ(25, (int)(vertices[0].environmentLight[0] * 100.0f));
    EXPECT_EQ(50, (int)(vertices[0].environmentLight[1] * 100.0f));
    EXPECT_EQ(75, (int)(vertices[0].environmentLight[2] * 100.0f));
    EXPECT_EQ(0, (int)vertices[0].depthBias);
    EXPECT_EQ(0, (int)vertices[0].shadowReception);

    storage[0].lightInfluence = 0.4f;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(40, (int)(vertices[0].lighting * 100.0f));

    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING |
                       RAGE_RENDER_INSTANCE_FLAT_SHADED;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(100, (int)(vertices[0].normal[0] * 100.0f));
    EXPECT_EQ(0, (int)(vertices[0].normal[1] * 100.0f));
    EXPECT_EQ(0, (int)(vertices[0].normal[2] * 100.0f));

    storage[0].assetSet = RAGE_RENDER_ASSET_COURSE;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(100, (int)(vertices[0].shadowReception * 100.0f));
    storage[0].assetSet = RAGE_RENDER_ASSET_MODEL_BANK;

    /* A native rear-view camera renders the ordinary main scene again. The
     * old PS1 mirror submissions must be independently selectable. */
    storage[1] = storage[0];
    storage[1].pass = RAGE_RENDER_PASS_MIRROR;
    world.instanceCount = 2;
    EXPECT_EQ(3, RenderBuildNativePassDraws(
                     &world, RAGE_RENDER_PASS_MAIN, 1.0f, test_mesh_lookup,
                     &mesh, vertices, 3, spans, 1, &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(RAGE_RENDER_PASS_MAIN, spans[0].pass);
    EXPECT_EQ(3, RenderBuildNativePassDraws(
                     &world, RAGE_RENDER_PASS_MIRROR, 1.0f, test_mesh_lookup,
                     &mesh, vertices, 3, spans, 1, &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(RAGE_RENDER_PASS_MIRROR, spans[0].pass);
}

static void test_native_draw_builder_rejects_invalid_inputs(void) {
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    uint32_t spanCount = 99;

    RenderWorldInit(&world, storage, 1);
    world.instanceCount = 1;
    world.instanceCapacity = 0;
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                        NULL, vertices, 3, spans, 1,
                                        &spanCount));
    EXPECT_EQ(0, spanCount);

    world.instanceCapacity = 1;
    world.instances = NULL;
    spanCount = 99;
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                        NULL, vertices, 3, spans, 1,
                                        &spanCount));
    EXPECT_EQ(0, spanCount);

    RenderWorldInit(&world, NULL, 0);
    spanCount = 99;
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, NAN, test_mesh_lookup,
                                        NULL, vertices, 3, spans, 1,
                                        &spanCount));
    EXPECT_EQ(0, spanCount);
}

static void test_native_draw_builder_keeps_triangles_for_gpu_frustum_clipping(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    /* One corner lies before the near plane. It must reach the GPU so the
     * rasterizer clips the triangle instead of a CPU projection dropping it. */
    float positions[3][3] = {{-1.0f, 0.0f, 0.5f},
                             {1.0f, 0.0f, 10.0f},
                             {0.0f, 1.0f, 10.0f}};
    unsigned i;
    uint32_t spanCount;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].transform.scale.x = 1.0f;
    storage[0].transform.scale.y = 1.0f;
    storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
}

static void test_native_draw_builder_culls_dynamic_course_backfaces(void) {
    unsigned char bytes[296] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[6];
    RageNativeDrawSpan spans[1];
    float positions[6][3] = {
        {-1.0f, -1.0f, -10.0f}, {0.0f, 1.0f, -10.0f},
        {1.0f, -1.0f, -10.0f},
        {-1.0f, -1.0f, -10.0f}, {1.0f, -1.0f, -10.0f},
        {0.0f, 1.0f, -10.0f},
    };
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 6); write_u32(bytes + 20, 6);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 6);
    for (i = 0; i < 6; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 272 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_COURSE;
    storage[0].flags = RAGE_RENDER_INSTANCE_CULL_BACKFACES;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 6, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
}

static void test_native_draw_builder_culls_terrain_per_authored_quad(void) {
    unsigned char bytes[216] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[6];
    RageNativeDrawSpan spans[1];
    const float visible[4][3] = {
        {1.0f, -1.0f, -10.0f}, {-1.0f, -1.0f, -10.0f},
        {1.0f, 1.0f, -10.0f}, {-1.0f, 1.0f, -10.0f},
    };
    const float hidden[4][3] = {
        {-1.0f, -1.0f, -10.0f}, {1.0f, -1.0f, -10.0f},
        {-1.0f, 1.0f, -10.0f}, {1.0f, 1.0f, -10.0f},
    };
    const uint32_t indices[6] = {0, 2, 1, 1, 2, 3};
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 4); write_u32(bytes + 20, 6);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 6);
    for (i = 0; i < 4; i++) {
        memcpy(bytes + 32 + i * 40, visible[i], sizeof(visible[i]));
        bytes[32 + i * 40 + 27] = 255;
    }
    for (i = 0; i < 6; i++) write_u32(bytes + 192 + i * 4, indices[i]);
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    /* Terrain uses the opposite imported winding from course objects. Both
     * triangles of its source quad remain visible together. */
    EXPECT_EQ(6, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 6, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    for (i = 0; i < 4; i++)
        memcpy(bytes + 32 + i * 40, hidden[i], sizeof(hidden[i]));
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 6, spans, 1,
                                             &spanCount));
    EXPECT_EQ(0, spanCount);

    /* A twisted authored quad stays whole when only its first half faces
     * the camera. Visibility must not become independent triangle culling. */
    for (i = 0; i < 4; i++)
        memcpy(bytes + 32 + i * 40, visible[i], sizeof(visible[i]));
    {
        float twisted[3] = {3.0f, -3.0f, -10.0f};
        memcpy(bytes + 32 + 3 * 40, twisted, sizeof(twisted));
    }
    EXPECT_EQ(6, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
        &mesh, vertices, 6, spans, 1, &spanCount));
    EXPECT_EQ(1, spanCount);

    /* A quad crossing the near plane reaches the GPU instead of being
     * rejected using pre-clip winding. */
    for (i = 0; i < 4; i++)
        memcpy(bytes + 32 + i * 40, visible[i], sizeof(visible[i]));
    {
        float crossing[3] = {1.0f, -1.0f, -0.5f};
        memcpy(bytes + 32, crossing, sizeof(crossing));
    }
    EXPECT_EQ(6, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 6, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);

    /* Clipping does not turn the hidden side of a wall into a giant polygon
     * when one of its corners passes behind the camera. */
    for (i = 0; i < 4; i++)
        memcpy(bytes + 32 + i * 40, hidden[i], sizeof(hidden[i]));
    {
        float crossing[3] = {-1.0f, -1.0f, -0.5f};
        memcpy(bytes + 32, crossing, sizeof(crossing));
    }
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 6, spans, 1,
                                             &spanCount));
    EXPECT_EQ(0, spanCount);
}

static void test_native_draw_builder_welds_terrain_cell_boundaries(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {
        {8191.0f, 15.0f, -8193.0f},
        {8193.0f, 16.0f, -8191.0f},
        {8188.0f, 17.0f, -8188.0f},
    };
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 10000.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 0.25f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(204800, (int)(vertices[0].position[0] * 100.0f));
    EXPECT_EQ(204800, (int)(vertices[1].position[0] * 100.0f));
    EXPECT_EQ(-204800, (int)(vertices[0].position[2] * 100.0f));
    EXPECT_EQ(-204800, (int)(vertices[1].position[2] * 100.0f));
    EXPECT_EQ(204700, (int)(vertices[2].position[0] * 100.0f));
    EXPECT_EQ(375, (int)(vertices[0].position[1] * 100.0f));
}

static void test_native_draw_builder_applies_authored_course_texture_scroll(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1.0f, 0.0f, 10.0f},
                             {1.0f, 0.0f, 10.0f},
                             {0.0f, 1.0f, 10.0f}};
    float uv[2] = {0.25f, 0.5f};
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        memcpy(bytes + 32 + i * 40 + 28, uv, sizeof(uv));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 32 + i * 40 + 36,
                  (uint32_t)RAGE_RUNTIME_MATERIAL_SCROLL_U | 4u);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].textureScrollU = 64;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(0, spans[0].depthDecal);
    EXPECT_EQ(50, (int)(vertices[0].uv[0] * 100.0f));
    EXPECT_EQ(50, (int)(vertices[0].uv[1] * 100.0f));
}

static void test_native_draw_builder_preserves_terrain_ot_bias(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1.0f, 0.0f, 10.0f},
                             {1.0f, 0.0f, 10.0f},
                             {0.0f, 1.0f, 10.0f}};
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 32 + i * 40 + 36,
                  RAGE_RUNTIME_MATERIAL_METADATA | (0xFCu <<
                      RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT) | 4u);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(0, spans[0].depthDecal);
    EXPECT_EQ(-4, (int)vertices[0].depthBias);
}

static void test_native_draw_builder_preserves_dynamic_terrain_material_flags(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1.0f, 0.0f, 10.0f},
                             {1.0f, 0.0f, 10.0f},
                             {0.0f, 1.0f, 10.0f}};
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 32 + i * 40 + 36,
                  RAGE_RUNTIME_MATERIAL_METADATA |
                  (0xFCu << RAGE_RUNTIME_MATERIAL_DEPTH_BIAS_SHIFT) |
                  RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY |
                  RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT | 4u);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 20000.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY |
                  RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT,
              spans[0].materialFlags);
    EXPECT_EQ(0, spans[0].depthDecal);
    EXPECT_EQ(-4, (int)vertices[0].depthBias);
    /* Both texture banks must retain their identity across lighting changes.
     * Only environment-sensitive faces may select the adjacent CLUT. */
    for (unsigned dynamic = 0; dynamic < 2; dynamic++) {
        for (i = 0; i < 3; i++)
            write_u32(bytes + 32 + i * 40 + 36,
                      RAGE_RUNTIME_MATERIAL_METADATA | 4u |
                      (dynamic ? RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT : 0));
        for (unsigned variant = 0; variant < 4; variant++) {
            storage[0].materialVariant = (uint8_t)variant;
            EXPECT_EQ(3, RenderBuildNativeDraws(
                &world, 1.0f, test_mesh_lookup, &mesh,
                vertices, 3, spans, 1, &spanCount));
            EXPECT_EQ(1, spanCount);
            EXPECT_EQ(dynamic ? variant : (variant & 2u),
                      spans[0].materialVariant);
            EXPECT_EQ(4, spans[0].material);
        }
    }
}

static void test_native_draw_builder_makes_road_paint_real_geometry(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {
        {0.0f, 0.0f, 10.0f}, {100.0f, 0.0f, 10.0f},
        {0.0f, 0.0f, 14.0f},
    };
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 32 + i * 40 + 36, 4);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 200.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(1, spans[0].depthDecal);
    EXPECT_EQ(200, (int)(vertices[0].position[1] * 100.0f));
    EXPECT_EQ(0, (int)vertices[0].depthBias);
}

static void test_native_draw_builder_keeps_terrain_detail_at_long_range(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1.0f, 0.0f, 0.0f},
                             {1.0f, 0.0f, 0.0f},
                             {0.0f, 1.0f, 0.0f}};
    uint32_t spanCount;
    unsigned i;

    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 32 + i * 40 + 36,
                  RAGE_RUNTIME_MATERIAL_METADATA |
                  RAGE_RUNTIME_MATERIAL_TERRAIN_NEAR_ONLY | 4u);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 20000.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.position.z = -11000.0f;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    /* The native renderer has a depth buffer and keeps imported detail. The
     * classic PS1 emitter still owns its far-cell simplification. */
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
}

static void test_native_draw_builder_culls_fully_offscreen_instance(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1.0f, 0.0f, 10.0f}, {1.0f, 0.0f, 10.0f},
                             {0.0f, 1.0f, 10.0f}};
    uint32_t spanCount;
    unsigned i;
    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255; write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].transform.position.x = 100.0f;
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(0, spanCount);
}

static void test_native_draw_builder_keeps_large_instance_crossing_frustum(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    /* The centre of this large triangle is beyond the right edge at its
     * depth, while its far corner is visible. Terrain cells near the camera
     * have this shape when scenery extends beyond the nominal cell square. */
    float positions[3][3] = {{15.0f, 0.0f, -20.0f},
                             {40.0f, -1.0f, 0.0f},
                             {40.0f, 1.0f, 0.0f}};
    uint32_t spanCount;
    unsigned i;
    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
}

static void test_native_draw_builder_keeps_instance_in_frustum_guard_band(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{10.8f, -0.1f, -10.0f},
                             {11.0f, -0.1f, -10.0f},
                             {10.9f, 0.1f, -10.0f}};
    uint32_t spanCount;
    unsigned i;
    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 3);
    for (i = 0; i < 3; i++) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        bytes[32 + i * 40 + 27] = 255;
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
}

/* Reference behaviour for moving view-dependent work out of mesh buffers.
 * Both views consume the SAME immutable asset and semantic MAIN instance. */
static void test_shared_mesh_independent_views(void) {
    unsigned char bytes[164] = {0}, original[164];
    RageRuntimeMesh mesh;
    RageRuntimeVertex source = {0};
    RageRenderMeshInstance instance = {0}, originalInstance;
    RageRenderWorld world, rear;
    RageNativeDrawVertex mainVertices[3], rearVertices[3], repeated[3];
    RageNativeDrawSpan spans[1];
    uint32_t spanCount;
    const float positions[3][3] = {
        {-1.0f, 0.0f, -10.0f}, {1.0f, 0.0f, -10.0f}, {0.0f, 1.0f, -10.0f}};
    EXPECT_EQ(1, RuntimeMeshEncodeHeader(bytes, sizeof(bytes), 1, 3, 3));
    write_u32(bytes + 24, 0);
    write_u32(bytes + 28, 3);
    source.normal[2] = 1.0f;
    source.color[0] = source.color[3] = 255;
    source.uv[0] = 0.125f;
    source.material = 4u | (uint32_t)RAGE_RUNTIME_MATERIAL_SCROLL_U;
    for (unsigned i = 0; i < 3; ++i) {
        memcpy(source.position, positions[i], sizeof(source.position));
        EXPECT_EQ(1, RuntimeVertexEncode(bytes + 32 + i * 40, 40, &source));
        write_u32(bytes + 152 + i * 4, i);
    }
    memcpy(original, bytes, sizeof(bytes));
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, &instance, 1);
    world.instanceCount = 1;
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f;
    world.camera.farPlane = 100.0f;
    world.camera.fogNear = 5.0f;
    world.camera.fogFar = 20.0f;
    world.camera.fogColor.x = 0.25f;
    instance.pass = RAGE_RENDER_PASS_MAIN;
    instance.assetSet = RAGE_RENDER_ASSET_COURSE;
    instance.transform.scale.x = instance.transform.scale.y =
        instance.transform.scale.z = 1.0f;
    instance.textureScrollU = 64;
    rear = world;
    rear.camera.transform.position.z = -30.0f;
    rear.camera.transform.rotation.y = 180.0f;
    rear.camera.fogColor.x = 0.75f;
    for (int overlay = 0; overlay < 2; ++overlay) {
        instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FOG |
            (overlay ? RAGE_RENDER_INSTANCE_DEPTH_DECAL : 0);
        originalInstance = instance;
        EXPECT_EQ(3, RenderBuildNativePassDraws(&world, RAGE_RENDER_PASS_MAIN,
            1.0f, test_mesh_lookup, &mesh, mainVertices, 3, spans, 1, &spanCount));
        EXPECT_EQ(1, spanCount);
        EXPECT_EQ(4, spans[0].material);
        EXPECT_EQ(3, RenderBuildNativePassDraws(&rear, RAGE_RENDER_PASS_MAIN,
            2.0f, test_mesh_lookup, &mesh, rearVertices, 3, spans, 1, &spanCount));
        EXPECT_EQ(1, spanCount);
        EXPECT_EQ(4, spans[0].material);
        EXPECT_EQ(3, RenderBuildNativePassDraws(&world, RAGE_RENDER_PASS_MAIN,
            1.0f, test_mesh_lookup, &mesh, repeated, 3, spans, 1, &spanCount));
        for (unsigned i = 0; i < 3; ++i) {
            EXPECT_NEAR(overlay ? -8.0f : -10.0f, mainVertices[i].position[2], 0.0001f);
            EXPECT_NEAR(overlay ? -12.0f : -10.0f, rearVertices[i].position[2], 0.0001f);
            /* Fog uses the authored position BEFORE decal lifting. */
            EXPECT_NEAR(2.0f / 3.0f, mainVertices[i].fog[3], 0.0001f);
            EXPECT_NEAR(1.0f, rearVertices[i].fog[3], 0.0001f);
            EXPECT_NEAR(0.25f, mainVertices[i].fog[0], 0.0001f);
            EXPECT_NEAR(0.75f, rearVertices[i].fog[0], 0.0001f);
            EXPECT_NEAR(0.375f, mainVertices[i].uv[0], 0.0001f);
            EXPECT_NEAR(0.375f, rearVertices[i].uv[0], 0.0001f);
            EXPECT_NEAR(mainVertices[i].position[2], repeated[i].position[2], 0.0001f);
            EXPECT_NEAR(mainVertices[i].fog[3], repeated[i].fog[3], 0.0001f);
            EXPECT_NEAR(mainVertices[i].uv[0], repeated[i].uv[0], 0.0001f);
        }
        EXPECT_EQ(3, RenderBuildNativeGpuPassDraws(&world, RAGE_RENDER_PASS_MAIN,
            1.0f, test_mesh_lookup, &mesh, repeated, 3, spans, 1, &spanCount));
        EXPECT_EQ(3, RenderBuildNativeGpuPassDraws(&rear, RAGE_RENDER_PASS_MAIN,
            2.0f, test_mesh_lookup, &mesh, rearVertices, 3, spans, 1, &spanCount));
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                EXPECT_NEAR(positions[i][axis], repeated[i].fog[axis], 0.0001f);
                EXPECT_NEAR(repeated[i].fog[axis], rearVertices[i].fog[axis], 0.0001f);
                EXPECT_NEAR(mainVertices[i].position[axis], repeated[i].position[axis], 0.0001f);
            }
            EXPECT_NEAR(1.0f, repeated[i].fog[3], 0.0001f);
            EXPECT_NEAR(1.0f, rearVertices[i].fog[3], 0.0001f);
            EXPECT_NEAR(mainVertices[i].uv[0], repeated[i].uv[0], 0.0001f);
        }
        EXPECT_EQ(0, memcmp(original, bytes, sizeof(bytes)));
        EXPECT_EQ(0, memcmp(&originalInstance, &instance, sizeof(instance)));
    }
    instance.flags = 0;
    EXPECT_EQ(3, RenderBuildNativeGpuPassDraws(&world, RAGE_RENDER_PASS_MAIN,
        1.0f, test_mesh_lookup, &mesh, repeated, 3, spans, 1, &spanCount));
    for (unsigned i = 0; i < 3; ++i)
        EXPECT_NEAR(0.0f, repeated[i].fog[3], 0.0001f);
}

static void test_gpu_vertex_reuse_preserves_instance_and_triangle_state(void) {
    unsigned char bytes[10348] = {0};
    RageRuntimeMesh mesh;
    RageRuntimeVertex vertex = {0};
    const uint32_t indices[] = {0, 1, 2, 0, 2, 256, 0, 1, 2};
    const float positions[4][3] = {{-1, 0, -10}, {1, 0, -10}, {0, 1, -10}, {-1, 1, -9}};
    RageRenderMeshInstance instances[2] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex reference[18], cached[18];
    RageNativeDrawSpan referenceSpans[2] = {0}, cachedSpans[2] = {0};
    uint32_t referenceCount, cachedCount;
    EXPECT_EQ(1, RuntimeMeshEncodeHeader(bytes, sizeof(bytes), 1, 257, 9));
    write_u32(bytes + 24, 0); write_u32(bytes + 28, 9);
    vertex.normal[1] = 1;
    vertex.color[0] = vertex.color[3] = 255;
    vertex.material = 4u | (uint32_t)RAGE_RUNTIME_MATERIAL_SCROLL_U;
    for (unsigned i = 0; i < 257; ++i) {
        memcpy(vertex.position, positions[i == 256 ? 3 : i % 3], sizeof(vertex.position));
        EXPECT_EQ(1, RuntimeVertexEncode(bytes + 32 + i * 40, 40, &vertex));
    }
    for (unsigned i = 0; i < 9; ++i) write_u32(bytes + 10312 + i * 4, indices[i]);
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, instances, 2);
    world.instanceCount = 2;
    world.camera.verticalFovDegrees = 90;
    world.camera.nearPlane = 1; world.camera.farPlane = 100;
    world.camera.fogNear = 5; world.camera.fogFar = 20;
    for (unsigned i = 0; i < 2; ++i) {
        instances[i].entity = i + 1;
        instances[i].pass = RAGE_RENDER_PASS_MAIN;
        instances[i].transform.scale = (RageRenderVec3){1, 1, 1};
        instances[i].transform.position.x = (float)i * 10;
        instances[i].textureScrollU = (uint8_t)(i * 64);
        instances[i].environmentLight = (RageRenderVec3){0.25f + (float)i * 0.5f, 0.5f, 1};
        instances[i].flags = RAGE_RENDER_INSTANCE_FLAT_SHADED |
            RAGE_RENDER_INSTANCE_DEPTH_DECAL | RAGE_RENDER_INSTANCE_ENABLE_FOG;
    }
    EXPECT_EQ(18, RenderBuildNativePassDraws(&world, RAGE_RENDER_PASS_MAIN,
        1, test_mesh_lookup, &mesh, reference, 18, referenceSpans, 2, &referenceCount));
    EXPECT_EQ(18, RenderBuildNativeGpuPassDraws(&world, RAGE_RENDER_PASS_MAIN,
        1, test_mesh_lookup, &mesh, cached, 18, cachedSpans, 2, &cachedCount));
    EXPECT_EQ(2, referenceCount); EXPECT_EQ(referenceCount, cachedCount);
    EXPECT_EQ(0, memcmp(referenceSpans, cachedSpans, sizeof(referenceSpans)));
    for (unsigned i = 0; i < 18; ++i) {
        RageRuntimeVertex original;
        EXPECT_EQ(1, RuntimeMeshVertex(&mesh, indices[i % 9], &original));
        EXPECT_NEAR(original.position[0] + (float)(i / 9) * 10, cached[i].fog[0], 0.0001f);
        EXPECT_NEAR(original.position[1], cached[i].fog[1], 0.0001f);
        EXPECT_NEAR(original.position[2], cached[i].fog[2], 0.0001f);
        EXPECT_NEAR(1, cached[i].fog[3], 0.0001f);
        /* Only the documented CPU/GPU fog encoding differs. Flat normals,
         * displaced positions, colours, UV scroll and instance light agree. */
        memcpy(reference[i].fog, cached[i].fog, sizeof(cached[i].fog));
        EXPECT_EQ(0, memcmp(&reference[i], &cached[i], sizeof(cached[i])));
    }

    /* Instance state is resolved anew on each build, including legacy zero
     * defaults. A partially zero environment colour must NOT become white. */
    for (unsigned asset = 0; asset <= 4; ++asset) {
        static const float influence[] = {-0.5f, 0.0f, 0.4f, 2.0f};
        static const float expected[] = {1.0f, 1.0f, 0.4f, 1.0f};
        for (unsigned state = 0; state < 4; ++state) {
            for (unsigned instance = 0; instance < 2; ++instance) {
                instances[instance].assetSet = (RageRenderAssetSet)asset;
                instances[instance].flags = instance == 0
                    ? RAGE_RENDER_INSTANCE_ENABLE_LIGHTING : 0;
                instances[instance].lightInfluence = influence[state];
                instances[instance].environmentLight = instance == 0
                    ? (RageRenderVec3){0, 0, 0} : (RageRenderVec3){0, 0.5f, 0};
            }
            for (unsigned gpu = 0; gpu < 2; ++gpu) {
                uint32_t count = gpu
                    ? RenderBuildNativeGpuPassDraws(&world, RAGE_RENDER_PASS_MAIN,
                        1, test_mesh_lookup, &mesh, cached, 18, cachedSpans, 2, &cachedCount)
                    : RenderBuildNativePassDraws(&world, RAGE_RENDER_PASS_MAIN,
                        1, test_mesh_lookup, &mesh, cached, 18, cachedSpans, 2, &cachedCount);
                EXPECT_EQ(18, count);
                RageNativeGpuVertex compact[18];
                RageNativeDrawSpan compactSpans[2] = {0};
                uint32_t compactSpanCount = 0;
                EXPECT_EQ(count, RenderBuildNativeCompactPassDraws(
                    &world, RAGE_RENDER_PASS_MAIN, 1, !gpu, test_mesh_lookup,
                    &mesh, compact, 18, compactSpans, 2, &compactSpanCount));
                EXPECT_EQ(cachedCount, compactSpanCount);
                EXPECT_EQ(0, memcmp(cachedSpans, compactSpans, sizeof(compactSpans)));
                for (unsigned i = 0; i < count; ++i) {
                    RageNativeGpuVertex expectedVertex = RenderPackNativeGpuVertex(&cached[i]);
                    EXPECT_EQ(0, memcmp(&expectedVertex, &compact[i], sizeof(expectedVertex)));
                    EXPECT_NEAR(i < 9 ? expected[state] : 0, cached[i].lighting, 0.0001f);
                    EXPECT_NEAR(i < 9 ? 1 : 0, cached[i].environmentLight[0], 0.0001f);
                    EXPECT_NEAR(i < 9 ? 1 : 0.5f, cached[i].environmentLight[1], 0.0001f);
                    EXPECT_NEAR(i < 9 ? 1 : 0, cached[i].environmentLight[2], 0.0001f);
                    EXPECT_NEAR(asset == 0 || asset == 3 ? 0 : 1,
                                cached[i].shadowReception, 0.0001f);
                }
            }
        }
    }
    /* Identical semantic IDs/materials may still carry different instance
     * lighting. They must not merge into one draw-constant uniform. */
    instances[0].flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING;
    instances[0].lightInfluence = 0.25f;
    instances[1] = instances[0];
    instances[1].lightInfluence = 0.75f;
    EXPECT_EQ(18, RenderBuildNativeGpuPassDraws(&world, RAGE_RENDER_PASS_MAIN,
        1, test_mesh_lookup, &mesh, cached, 18, cachedSpans, 2, &cachedCount));
    EXPECT_EQ(2, cachedCount);
    EXPECT_NEAR(0.25f, cachedSpans[0].instanceState.lighting, 0.0001f);
    EXPECT_NEAR(0.75f, cachedSpans[1].instanceState.lighting, 0.0001f);
    EXPECT_EQ(9, cachedSpans[0].vertexCount);
    EXPECT_EQ(9, cachedSpans[1].firstVertex);
}

static void test_gpu_vertex_payload_excludes_instance_state(void) {
    RageNativeDrawVertex source = {0};
    for (unsigned i = 0; i < 3; ++i) {
        source.position[i] = (float)i - 1.25f;
        source.normal[i] = (float)i * 0.25f;
    }
    source.uv[0] = -0.5f; source.uv[1] = 2.25f;
    for (unsigned i = 0; i < 4; ++i) {
        source.color[i] = (uint8_t)(i * 71);
        source.fog[i] = (float)i - 8.5f;
    }
    source.depthBias = -17.25f;
    RageNativeGpuVertex packed = RenderPackNativeGpuVertex(&source);
    EXPECT_EQ(56, sizeof(packed));
    EXPECT_EQ(0, memcmp(packed.position, source.position, sizeof(source.position)));
    EXPECT_EQ(0, memcmp(packed.normal, source.normal, sizeof(source.normal)));
    EXPECT_EQ(0, memcmp(packed.uv, source.uv, sizeof(source.uv)));
    EXPECT_EQ(0, memcmp(packed.color, source.color, sizeof(source.color)));
    EXPECT_EQ(0, memcmp(packed.fog, source.fog, sizeof(source.fog)));
    EXPECT_NEAR(source.depthBias, packed.depthBias, 0.0001f);
    source.lighting = 0.75f;
    source.environmentLight[0] = 0.25f;
    source.environmentLight[1] = 0.5f;
    source.environmentLight[2] = 1;
    source.shadowReception = 1;
    RageNativeGpuVertex changedInstance = RenderPackNativeGpuVertex(&source);
    EXPECT_EQ(0, memcmp(&packed, &changedInstance, sizeof(packed)));
}

static void test_overlay_orientation_and_degenerate_geometry(void) {
    const float scales[] = {1.0f, 0.0001f, 0.0f};
    const float cameras[] = {0.0f, -20.0f, -10.0f};
    for (unsigned shape = 0; shape < 3; ++shape)
    for (unsigned reverse = 0; reverse < 2; ++reverse) {
        unsigned char bytes[164] = {0};
        RageRuntimeMesh mesh;
        RageRuntimeVertex vertex = {0};
        RageRenderMeshInstance instance = {0};
        RageRenderWorld world;
        RageNativeGpuVertex output[3];
        RageNativeDrawSpan span;
        uint32_t spanCount;
        EXPECT_EQ(1, RuntimeMeshEncodeHeader(bytes, sizeof(bytes), 1, 3, 3));
        write_u32(bytes + 28, 3);
        vertex.normal[1] = 1;
        vertex.position[2] = -10;
        for (unsigned i = 0; i < 3; ++i) {
            vertex.position[0] = i == 1 ? scales[shape] : 0;
            vertex.position[1] = i == 2 ? scales[shape] : 0;
            EXPECT_EQ(1, RuntimeVertexEncode(bytes + 32 + i * 40, 40, &vertex));
            write_u32(bytes + 152 + i * 4, reverse && i != 0 ? 3 - i : i);
        }
        EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
        RenderWorldInit(&world, &instance, 1);
        world.instanceCount = 1;
        world.camera.verticalFovDegrees = 90;
        world.camera.nearPlane = 1; world.camera.farPlane = 100;
        instance.pass = RAGE_RENDER_PASS_MAIN;
        instance.transform.scale = (RageRenderVec3){1, 1, 1};
        instance.flags = RAGE_RENDER_INSTANCE_FLAT_SHADED |
            RAGE_RENDER_INSTANCE_DEPTH_DECAL | RAGE_RENDER_INSTANCE_ENABLE_FOG;
        for (unsigned view = 0; view < 3; ++view) {
            world.camera.transform.position.z = cameras[view];
            EXPECT_EQ(3, RenderBuildNativeCompactPassDraws(&world,
                RAGE_RENDER_PASS_MAIN, 1, 0, test_mesh_lookup, &mesh,
                output, 3, &span, 1, &spanCount));
            EXPECT_EQ(1, spanCount);
            float lift = shape == 2 ? 0 : view == 0 ? 2 : view == 1 ? -2 : reverse ? -2 : 2;
            for (unsigned i = 0; i < 3; ++i) {
                EXPECT_NEAR(-10 + lift, output[i].position[2], 0.0001f);
                EXPECT_NEAR(-10, output[i].fog[2], 0.0001f);
                /* Flat shading keeps its original epsilon; tiny nonzero
                 * faces can still lift without replacing authored normals. */
                EXPECT_NEAR(shape == 0 ? 0 : 1, output[i].normal[1], 0.0001f);
                EXPECT_NEAR(shape == 0 ? (reverse ? -1 : 1) : 0,
                            output[i].normal[2], 0.0001f);
            }
        }
    }
}

static void test_car_marking_stays_outside_hood(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeDrawSpan spans[1];
    float positions[3][3] = {{-1, 0, 10}, {1, 0, 10}, {0, 0, 12}};
    float normal[3] = {0, 1, 0};
    uint32_t spanCount, i;
    uint32_t material = 2 + RAGE_CAR_SURFACE_DECAL * RAGE_CAR_SURFACE_RUNTIME_STRIDE;
    memcpy(bytes, "RRMESH1", 7);
    write_u32(bytes + 8, 1); write_u32(bytes + 12, 1);
    write_u32(bytes + 16, 3); write_u32(bytes + 20, 3);
    write_u32(bytes + 28, 3);
    for (i = 0; i < 3; ++i) {
        memcpy(bytes + 32 + i * 40, positions[i], sizeof(positions[i]));
        memcpy(bytes + 44 + i * 40, normal, sizeof(normal));
        bytes[59 + i * 40] = 255;
        write_u32(bytes + 68 + i * 40, RAGE_RUNTIME_MATERIAL_METADATA | material);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RenderWorldInit(&world, storage, 1);
    world.camera.transform.position.y = -1; /* Below the hood plane. */
    world.camera.verticalFovDegrees = 90;
    world.camera.nearPlane = 1; world.camera.farPlane = 100;
    storage[0].assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1;
    world.instanceCount = 1;
    EXPECT_EQ(3, RenderBuildNativeDraws(&world, 1, test_mesh_lookup,
        &mesh, vertices, 3, spans, 1, &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(1, spans[0].depthDecal);
    EXPECT_EQ(material, spans[0].material);
    for (i = 0; i < 3; ++i) {
        EXPECT_EQ(2, (int)vertices[i].position[1]);
        EXPECT_EQ(0, (int)vertices[i].depthBias);
    }
}

static const RageRuntimeMesh *count_missing_lookup(
    void *context, const RageRenderMeshInstance *instance) {
    unsigned *calls = context;
    ++calls[instance->pass == RAGE_RENDER_PASS_MAIN ? 0 : 1];
    return NULL;
}

static void test_pass_filter_precedes_asset_lookup(void) {
    RageRenderMeshInstance instances[2] = {0};
    RageRenderWorld world;
    RageNativeDrawVertex vertices[3];
    RageNativeGpuVertex compact[3];
    RageNativeDrawSpan spans[1];
    uint32_t spanCount;
    unsigned calls[2] = {0};
    RenderWorldInit(&world, instances, 2);
    instances[0].pass = RAGE_RENDER_PASS_MAIN;
    instances[1].pass = RAGE_RENDER_PASS_MIRROR;
    world.instanceCount = 2;
    EXPECT_EQ(0, RenderBuildNativePassDraws(&world, RAGE_RENDER_PASS_MAIN,
        1, count_missing_lookup, calls, vertices, 3, spans, 1, &spanCount));
    EXPECT_EQ(1, calls[0]); EXPECT_EQ(0, calls[1]);
    EXPECT_EQ(0, spanCount);
    EXPECT_EQ(0, RenderBuildNativeGpuPassDraws(&world, RAGE_RENDER_PASS_MIRROR,
        1, count_missing_lookup, calls, vertices, 3, spans, 1, &spanCount));
    EXPECT_EQ(1, calls[0]); EXPECT_EQ(1, calls[1]);
    EXPECT_EQ(0, RenderBuildNativeCompactPassDraws(&world, RAGE_RENDER_PASS_MAIN,
        1, 0, count_missing_lookup, calls, compact, 3, spans, 1, &spanCount));
    EXPECT_EQ(2, calls[0]); EXPECT_EQ(1, calls[1]);
    EXPECT_EQ(0, RenderBuildNativeDraws(&world, 1, count_missing_lookup,
        calls, vertices, 3, spans, 1, &spanCount));
    EXPECT_EQ(3, calls[0]); EXPECT_EQ(2, calls[1]);
}

int main(void) {
    test_pass_filter_precedes_asset_lookup();
    test_overlay_orientation_and_degenerate_geometry();
    test_gpu_vertex_payload_excludes_instance_state();
    test_gpu_vertex_reuse_preserves_instance_and_triangle_state();
    test_shared_mesh_independent_views();
    test_car_marking_stays_outside_hood();
    test_native_draw_builder_uses_render_world_and_imported_mesh();
    test_native_draw_builder_rejects_invalid_inputs();
    test_native_draw_builder_keeps_triangles_for_gpu_frustum_clipping();
    test_native_draw_builder_culls_dynamic_course_backfaces();
    test_native_draw_builder_culls_terrain_per_authored_quad();
    test_native_draw_builder_welds_terrain_cell_boundaries();
    test_native_draw_builder_applies_authored_course_texture_scroll();
    test_native_draw_builder_preserves_terrain_ot_bias();
    test_native_draw_builder_preserves_dynamic_terrain_material_flags();
    test_native_draw_builder_makes_road_paint_real_geometry();
    test_native_draw_builder_keeps_terrain_detail_at_long_range();
    test_native_draw_builder_culls_fully_offscreen_instance();
    test_native_draw_builder_keeps_large_instance_crossing_frustum();
    test_native_draw_builder_keeps_instance_in_frustum_guard_band();
    if (failures != 0) return EXIT_FAILURE;
    puts("render mesh build tests passed");
    return EXIT_SUCCESS;
}
