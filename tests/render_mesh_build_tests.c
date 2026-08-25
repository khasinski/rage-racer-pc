#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/render_mesh_build.h"

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

static void test_native_draw_builder_uses_render_world_and_imported_mesh(void) {
    unsigned char bytes[164] = {0};
    RageRuntimeMesh mesh;
    RageRenderMeshInstance storage[1] = {0};
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
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
    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(10, spans[0].assetKey);
    EXPECT_EQ(RAGE_RENDER_ASSET_MODEL_BANK, spans[0].assetSet);
    EXPECT_EQ(0, spans[0].depthDecal);
    EXPECT_EQ(1, spans[0].materialVariant);
    /* Per-instance basis matches the old X/Y/Z rotation order without
     * recalculating trigonometry for every emitted vertex. */
    EXPECT_EQ(1000, (int)(vertices[0].position[0] * 100.0f));
    EXPECT_EQ(99, (int)(vertices[0].position[2] * 100.0f));
    EXPECT_EQ(200, vertices[0].color[2]);
    EXPECT_EQ(100, (int)(vertices[0].normal[1] * 100.0f));
    EXPECT_EQ(25, (int)(vertices[0].fog[0] * 100.0f));
    EXPECT_EQ(50, (int)(vertices[0].fog[1] * 100.0f));
    EXPECT_EQ(75, (int)(vertices[0].fog[2] * 100.0f));
    EXPECT_EQ(0, (int)(vertices[0].fog[3] * 100.0f));
    EXPECT_EQ(0, (int)(vertices[0].lighting * 100.0f));
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING;
    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(100, (int)(vertices[0].lighting * 100.0f));
    EXPECT_EQ(25, (int)(vertices[0].environmentLight[0] * 100.0f));
    EXPECT_EQ(50, (int)(vertices[0].environmentLight[1] * 100.0f));
    EXPECT_EQ(75, (int)(vertices[0].environmentLight[2] * 100.0f));
    EXPECT_EQ(-16, (int)vertices[0].depthBias);
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].transform.scale.x = 1.0f;
    storage[0].transform.scale.y = 1.0f;
    storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
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
                  RAGE_RUNTIME_MATERIAL_SCROLL_U | 4u);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].textureScrollU = 64;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(0, spans[0].depthDecal);
    EXPECT_EQ(50, (int)(vertices[0].uv[0] * 100.0f));
    EXPECT_EQ(50, (int)(vertices[0].uv[1] * 100.0f));
}

static void test_native_draw_builder_strips_ot_bias_from_material_lookup(void) {
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(1, spans[0].depthDecal);
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
                  RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT | 4u);
        write_u32(bytes + 152 + i * 4, i);
    }
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 20000.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
    EXPECT_EQ(4, spans[0].material);
    EXPECT_EQ(RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT,
              spans[0].materialFlags);
    EXPECT_EQ(1, spans[0].depthDecal);
    EXPECT_EQ(-4, (int)vertices[0].depthBias);
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 20000.0f;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].transform.position.z = -11000.0f;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;

    /* The native renderer has a depth buffer and keeps imported detail. The
     * classic PS1 emitter still owns its far-cell simplification. */
    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].transform.position.x = 100.0f;
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(0, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
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
    EXPECT_EQ(1, RageRuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
    RageRenderWorldInit(&world, storage, 1);
    world.camera.verticalFovDegrees = 90.0f;
    world.camera.nearPlane = 1.0f; world.camera.farPlane = 100.0f;
    storage[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL;
    storage[0].transform.scale.x = storage[0].transform.scale.y =
        storage[0].transform.scale.z = 1.0f;
    world.instanceCount = 1;
    EXPECT_EQ(3, RageRenderBuildNativeDraws(&world, 1.0f, test_mesh_lookup,
                                             &mesh, vertices, 3, spans, 1,
                                             &spanCount));
    EXPECT_EQ(1, spanCount);
}

int main(void) {
    test_native_draw_builder_uses_render_world_and_imported_mesh();
    test_native_draw_builder_keeps_triangles_for_gpu_frustum_clipping();
    test_native_draw_builder_applies_authored_course_texture_scroll();
    test_native_draw_builder_strips_ot_bias_from_material_lookup();
    test_native_draw_builder_preserves_dynamic_terrain_material_flags();
    test_native_draw_builder_keeps_terrain_detail_at_long_range();
    test_native_draw_builder_culls_fully_offscreen_instance();
    test_native_draw_builder_keeps_large_instance_crossing_frustum();
    test_native_draw_builder_keeps_instance_in_frustum_guard_band();
    if (failures != 0) return EXIT_FAILURE;
    puts("render mesh build tests passed");
    return EXIT_SUCCESS;
}
