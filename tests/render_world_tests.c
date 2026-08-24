#include <stdio.h>
#include <stdlib.h>

#include "render/render_world.h"
#include "render/render_world_frame.h"
#include "render/render_projection.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                      \
    unsigned long long expected_value = (unsigned long long)(expected);       \
    unsigned long long actual_value = (unsigned long long)(actual);           \
    if (expected_value != actual_value) {                                     \
        fprintf(stderr, "%s:%d: expected %llu, got %llu\\n", __FILE__,      \
                __LINE__, expected_value, actual_value);                      \
        failures++;                                                            \
    }                                                                          \
} while (0)

static void test_frame_reset_preserves_storage_and_resets_overflow(void) {
    RageRenderMeshInstance storage[1];
    RageRenderWorld world;
    RageRenderMeshInstance instance = {0};

    RageRenderWorldInit(&world, storage, 1);
    RageRenderWorldBeginFrame(&world, 41);
    instance.entity = 7;
    instance.mesh = 12;
    instance.assetSet = RAGE_RENDER_ASSET_COURSE;
    instance.assetKey = 0x101;
    instance.material = 3;
    instance.pass = RAGE_RENDER_PASS_MIRROR;
    instance.transform.position.x = 22.0f;
    instance.previousTransform.position.x = 20.0f;
    EXPECT_EQ(1, RageRenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(0, RageRenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(1, world.instanceCount);
    EXPECT_EQ(1, world.overflowCount);
    EXPECT_EQ(7, storage[0].entity);
    EXPECT_EQ(RAGE_RENDER_ASSET_COURSE, storage[0].assetSet);
    EXPECT_EQ(0x101, storage[0].assetKey);
    EXPECT_EQ(RAGE_RENDER_PASS_MIRROR, storage[0].pass);

    RageRenderWorldBeginFrame(&world, 42);
    EXPECT_EQ(42, world.frame);
    EXPECT_EQ(0, world.instanceCount);
    EXPECT_EQ(0, world.overflowCount);
    EXPECT_EQ(1, RageRenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(20, (int)storage[0].previousTransform.position.x);
}

static void test_camera_is_scene_data_not_backend_state(void) {
    RageRenderMeshInstance storage[2];
    RageRenderWorld world;
    RageRenderCamera camera = {0};

    RageRenderWorldInit(&world, storage, 2);
    camera.transform.position.y = 3.5f;
    camera.verticalFovDegrees = 70.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 5000.0f;
    RageRenderWorldSetCamera(&world, &camera);
    EXPECT_EQ(35, (int)(world.camera.transform.position.y * 10.0f));
    EXPECT_EQ(70, (int)world.camera.verticalFovDegrees);
    EXPECT_EQ(5000, (int)world.camera.farPlane);

    RageRenderWorldBeginFrame(&world, 2);
    camera.transform.position.y = 13.5f;
    camera.verticalFovDegrees = 80.0f;
    RageRenderWorldSetCamera(&world, &camera);
    RageRenderInterpolateCamera(&world.previousCamera, &world.camera, 0.5f,
                                &camera);
    EXPECT_EQ(85, (int)(camera.transform.position.y * 10.0f));
    EXPECT_EQ(75, (int)camera.verticalFovDegrees);
}

static void test_terrain_grid_places_adjacent_cells_without_overlap(void) {
    RageRenderTransform left;
    RageRenderTransform right;
    RageRenderTransform south;

    RageRenderTerrainCellTransform(0, 0, &left);
    RageRenderTerrainCellTransform(1, 0, &right);
    RageRenderTerrainCellTransform(0, 1, &south);
    EXPECT_EQ(4096, (int)left.position.x);
    EXPECT_EQ(258048, (int)left.position.z);
    EXPECT_EQ(8192, (int)(right.position.x - left.position.x));
    EXPECT_EQ(-8192, (int)(south.position.z - left.position.z));
    EXPECT_EQ(1, (int)left.scale.x);
}

static void test_presentation_interpolates_without_mutating_game_world(void) {
    RageRenderMeshInstance storage[2] = {0};
    RageRenderMeshInstance presentation[2] = {0};
    RageRenderWorld world;

    RageRenderWorldInit(&world, storage, 2);
    RageRenderWorldBeginFrame(&world, 1);
    storage[0].entity = 9;
    storage[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    storage[0].assetKey = 0x503;
    storage[0].pass = RAGE_RENDER_PASS_MIRROR;
    storage[0].previousTransform.position.x = 10.0f;
    storage[0].transform.position.x = 30.0f;
    storage[0].previousTransform.rotation.y = 350.0f;
    storage[0].transform.rotation.y = 10.0f;
    world.instanceCount = 1;
    EXPECT_EQ(1, RageRenderWorldBuildPresentation(&world, 0.5f,
                                                   presentation, 2));
    EXPECT_EQ(9, presentation[0].entity);
    EXPECT_EQ(RAGE_RENDER_ASSET_TERRAIN, presentation[0].assetSet);
    EXPECT_EQ(0x503, presentation[0].assetKey);
    EXPECT_EQ(RAGE_RENDER_PASS_MIRROR, presentation[0].pass);
    EXPECT_EQ(20, (int)presentation[0].transform.position.x);
    /* 350 -> 10 takes the 20 degree path, not a full 340 degree spin. */
    EXPECT_EQ(360, (int)presentation[0].transform.rotation.y);
    EXPECT_EQ(30, (int)storage[0].transform.position.x);
}

static void test_native_camera_projection_has_no_gte_quantization(void) {
    RageRenderCamera camera = {0};
    RageRenderVec3 world = {10.0f, 5.0f, 100.0f};
    RageRenderVec3 view;
    RageRenderVec3 clip;

    camera.verticalFovDegrees = 90.0f;
    camera.nearPlane = 1.0f;
    camera.farPlane = 1000.0f;
    RageRenderWorldToView(&camera, &world, &view);
    EXPECT_EQ(10, (int)view.x);
    EXPECT_EQ(100, (int)view.z);
    EXPECT_EQ(1, RageRenderProject(&camera, &view, 2.0f, &clip));
    EXPECT_EQ(5, (int)(clip.x * 100.0f));
    EXPECT_EQ(5, (int)(clip.y * 100.0f));
    camera.transform.rotation.y = 90.0f;
    world.x = 100.0f; world.z = 0.0f;
    RageRenderWorldToView(&camera, &world, &view);
    EXPECT_EQ(100, (int)view.z);
}

static void test_psx_rotation_uses_the_same_basis_as_imported_positions(void) {
    const float source[3][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
    };
    float converted[3][3];
    RageRenderConvertPsxMatrix(source, converted);
    EXPECT_EQ(-1, (int)converted[0][2]);
    EXPECT_EQ(1, (int)converted[1][1]);
    EXPECT_EQ(1, (int)converted[2][0]);
}

int main(void) {
    test_frame_reset_preserves_storage_and_resets_overflow();
    test_camera_is_scene_data_not_backend_state();
    test_terrain_grid_places_adjacent_cells_without_overlap();
    test_presentation_interpolates_without_mutating_game_world();
    test_native_camera_projection_has_no_gte_quantization();
    test_psx_rotation_uses_the_same_basis_as_imported_positions();
    if (failures != 0) return EXIT_FAILURE;
    puts("render world tests passed");
    return EXIT_SUCCESS;
}
