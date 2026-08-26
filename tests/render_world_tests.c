#include <stdio.h>
#include <stdlib.h>

#include "render/render_world.h"
#include "render/render_world_frame.h"
#include "render/render_projection.h"
#include "render/render_shadow.h"

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

static void test_legacy_mirror_instances_can_be_removed_from_scene(void) {
    RageRenderMeshInstance storage[3] = {0};
    RageRenderWorld world;

    RageRenderWorldInit(&world, storage, 3);
    storage[0].entity = 10;
    storage[0].pass = RAGE_RENDER_PASS_MAIN;
    storage[1].entity = 20;
    storage[1].pass = RAGE_RENDER_PASS_MIRROR;
    storage[2].entity = 30;
    storage[2].pass = RAGE_RENDER_PASS_MAIN;
    world.instanceCount = 3;
    RageRenderWorldDiscardPass(&world, RAGE_RENDER_PASS_MIRROR);
    EXPECT_EQ(2, world.instanceCount);
    EXPECT_EQ(10, storage[0].entity);
    EXPECT_EQ(30, storage[1].entity);
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
    camera.fogColor.x = 0.25f;
    camera.skyColor.x = 0.20f;
    camera.fogNear = 100.0f;
    camera.fogFar = 500.0f;
    RageRenderWorldSetCamera(&world, &camera);
    EXPECT_EQ(35, (int)(world.camera.transform.position.y * 10.0f));
    EXPECT_EQ(70, (int)world.camera.verticalFovDegrees);
    EXPECT_EQ(5000, (int)world.camera.farPlane);
    EXPECT_EQ(25, (int)(world.camera.fogColor.x * 100.0f));
    EXPECT_EQ(20, (int)(world.camera.skyColor.x * 100.0f));
    EXPECT_EQ(500, (int)world.camera.fogFar);

    RageRenderWorldBeginFrame(&world, 2);
    camera.transform.position.y = 13.5f;
    camera.verticalFovDegrees = 80.0f;
    camera.fogColor.x = 0.75f;
    camera.skyColor.x = 0.80f;
    camera.fogNear = 200.0f;
    camera.fogFar = 1000.0f;
    RageRenderWorldSetCamera(&world, &camera);
    RageRenderInterpolateCamera(&world.previousCamera, &world.camera, 0.5f,
                                &camera);
    EXPECT_EQ(85, (int)(camera.transform.position.y * 10.0f));
    EXPECT_EQ(75, (int)camera.verticalFovDegrees);
    EXPECT_EQ(50, (int)(camera.fogColor.x * 100.0f));
    EXPECT_EQ(50, (int)(camera.skyColor.x * 100.0f));
    EXPECT_EQ(150, (int)camera.fogNear);
    EXPECT_EQ(750, (int)camera.fogFar);
}

static void test_mirror_is_an_independent_scene_camera(void) {
    RageRenderMeshInstance storage[1];
    RageRenderWorld world;
    RageRenderCamera camera = {0};

    RageRenderWorldInit(&world, storage, 1);
    camera.transform.position.z = 100.0f;
    camera.transform.rotation.y = 180.0f;
    camera.verticalFovDegrees = 20.0f;
    RageRenderWorldSetMirrorCamera(&world, &camera, 1, -12.0f);
    EXPECT_EQ(1, world.hasMirrorCamera);
    EXPECT_EQ(1, world.mirrorActive);
    EXPECT_EQ(180, (int)world.mirrorCamera.transform.rotation.y);
    EXPECT_EQ(20, (int)world.mirrorCamera.verticalFovDegrees);
    EXPECT_EQ(-12, (int)world.mirrorPanelY);

    RageRenderWorldBeginFrame(&world, 2);
    camera.transform.position.z = 140.0f;
    RageRenderWorldSetMirrorCamera(&world, &camera, 0, 18.0f);
    RageRenderInterpolateCamera(&world.previousMirrorCamera,
                                &world.mirrorCamera, 0.5f, &camera);
    EXPECT_EQ(120, (int)camera.transform.position.z);
    EXPECT_EQ(0, world.mirrorActive);
    EXPECT_EQ(-12, (int)world.previousMirrorPanelY);
    EXPECT_EQ(18, (int)world.mirrorPanelY);
}

static void test_perspective_fog_uses_authored_near_and_far_depths(void) {
    RageRenderCamera camera = {0};
    RageRenderVec3 point = {0.0f, 0.0f, -10.0f};
    camera.fogNear = 10.0f;
    camera.fogFar = 50.0f;
    EXPECT_EQ(0, (int)(RageRenderFogFactor(&camera, &point) * 100.0f));
    point.z = -25.0f;
    EXPECT_EQ(75, (int)(RageRenderFogFactor(&camera, &point) * 100.0f));
    point.z = -50.0f;
    EXPECT_EQ(100, (int)(RageRenderFogFactor(&camera, &point) * 100.0f));
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

static void test_synchronized_presentation_keeps_previous_vehicle_models(void) {
    RageRenderMeshInstance previousStorage[3] = {0};
    RageRenderMeshInstance currentStorage[3] = {0};
    RageRenderMeshInstance presentation[4] = {0};
    RageRenderWorld previous;
    RageRenderWorld current;
    uint32_t count;

    RageRenderWorldInit(&previous, previousStorage, 3);
    RageRenderWorldInit(&current, currentStorage, 3);
    previousStorage[0].entity = 10;
    previousStorage[0].mesh = 30;
    previousStorage[0].assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    previousStorage[0].assetKey = 88;
    previousStorage[0].materialVariant = 29;
    previousStorage[0].transform.position.x = 10.0f;
    previous.instanceCount = 1;

    /* The next intro tick sees another rival. It must not replace the model
     * while presentation is still compositing the previous logic frame. */
    currentStorage[0].entity = 3;
    currentStorage[0].mesh = 15;
    currentStorage[0].assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    currentStorage[0].assetKey = 88;
    currentStorage[0].materialVariant = 27;
    currentStorage[0].transform.position.x = 100.0f;
    currentStorage[1].entity = 0x10000;
    currentStorage[1].mesh = 4;
    currentStorage[1].assetSet = RAGE_RENDER_ASSET_COURSE;
    currentStorage[1].assetKey = 88;
    currentStorage[1].previousTransform.position.x = 20.0f;
    currentStorage[1].transform.position.x = 40.0f;
    current.instanceCount = 2;

    count = RageRenderWorldBuildSynchronizedPresentation(
        &previous, &current, 0.5f, presentation, 4);
    EXPECT_EQ(2, count);
    EXPECT_EQ(RAGE_RENDER_ASSET_COURSE, presentation[0].assetSet);
    EXPECT_EQ(30, (int)presentation[0].transform.position.x);
    EXPECT_EQ(RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
              presentation[1].assetSet);
    EXPECT_EQ(10, presentation[1].entity);
    EXPECT_EQ(30, presentation[1].mesh);
    EXPECT_EQ(10, (int)presentation[1].transform.position.x);
}

static void test_synchronized_presentation_moves_matching_vehicle(void) {
    RageRenderMeshInstance previousStorage[1] = {0};
    RageRenderMeshInstance currentStorage[1] = {0};
    RageRenderMeshInstance presentation[1] = {0};
    RageRenderWorld previous;
    RageRenderWorld current;

    RageRenderWorldInit(&previous, previousStorage, 1);
    RageRenderWorldInit(&current, currentStorage, 1);
    previousStorage[0].entity = currentStorage[0].entity = 7;
    previousStorage[0].mesh = currentStorage[0].mesh = 25;
    previousStorage[0].assetSet = currentStorage[0].assetSet =
        RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    previousStorage[0].assetKey = currentStorage[0].assetKey = 88;
    previousStorage[0].materialVariant = currentStorage[0].materialVariant = 28;
    previousStorage[0].transform.position.z = 100.0f;
    currentStorage[0].transform.position.z = 140.0f;
    previous.instanceCount = current.instanceCount = 1;

    EXPECT_EQ(1, RageRenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.25f, presentation, 1));
    EXPECT_EQ(25, presentation[0].mesh);
    EXPECT_EQ(110, (int)presentation[0].transform.position.z);
}

static void test_synchronized_presentation_keeps_wheel_sides_paired(void) {
    RageRenderMeshInstance previousStorage[2] = {0};
    RageRenderMeshInstance currentStorage[2] = {0};
    RageRenderMeshInstance presentation[2] = {0};
    RageRenderWorld previous;
    RageRenderWorld current;
    unsigned i;

    RageRenderWorldInit(&previous, previousStorage, 2);
    RageRenderWorldInit(&current, currentStorage, 2);
    for (i = 0; i < 2; i++) {
        previousStorage[i].entity = currentStorage[i].entity = 11;
        previousStorage[i].mesh = currentStorage[i].mesh = 14;
        previousStorage[i].assetSet = currentStorage[i].assetSet =
            RAGE_RENDER_ASSET_MODEL_BANK;
        previousStorage[i].assetKey = currentStorage[i].assetKey = 20;
        previousStorage[i].component = currentStorage[i].component =
            (uint8_t)(3 + i);
    }
    previousStorage[0].transform.position.x = -10.0f;
    previousStorage[1].transform.position.x = 10.0f;
    currentStorage[0].transform.position.x = 12.0f;
    currentStorage[1].transform.position.x = -12.0f;
    previous.instanceCount = current.instanceCount = 2;

    EXPECT_EQ(2, RageRenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.5f, presentation, 2));
    EXPECT_EQ(3, presentation[0].component);
    EXPECT_EQ(1, (int)presentation[0].transform.position.x);
    EXPECT_EQ(4, presentation[1].component);
    EXPECT_EQ(-1, (int)presentation[1].transform.position.x);
}

static void test_synchronized_presentation_matches_animated_wheel_mesh(void) {
    RageRenderMeshInstance previousStorage[1] = {0};
    RageRenderMeshInstance currentStorage[1] = {0};
    RageRenderMeshInstance presentation[1] = {0};
    RageRenderWorld previous;
    RageRenderWorld current;

    RageRenderWorldInit(&previous, previousStorage, 1);
    RageRenderWorldInit(&current, currentStorage, 1);
    previousStorage[0].entity = currentStorage[0].entity = 11;
    previousStorage[0].component = currentStorage[0].component = 2;
    previousStorage[0].mesh = 3;
    currentStorage[0].mesh = 13;
    previousStorage[0].assetSet = currentStorage[0].assetSet =
        RAGE_RENDER_ASSET_MODEL_BANK;
    previousStorage[0].assetKey = currentStorage[0].assetKey = 20;
    previousStorage[0].transform.position.z = 100.0f;
    currentStorage[0].transform.position.z = 140.0f;
    previous.instanceCount = current.instanceCount = 1;

    EXPECT_EQ(1, RageRenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.25f, presentation, 1));
    EXPECT_EQ(3, presentation[0].mesh);
    EXPECT_EQ(110, (int)presentation[0].transform.position.z);
}

static void test_synchronized_presentation_moves_dynamic_scenery(void) {
    RageRenderMeshInstance previousStorage[1] = {0};
    RageRenderMeshInstance currentStorage[1] = {0};
    RageRenderMeshInstance presentation[1] = {0};
    RageRenderWorld previous;
    RageRenderWorld current;

    RageRenderWorldInit(&previous, previousStorage, 1);
    RageRenderWorldInit(&current, currentStorage, 1);
    previousStorage[0].entity = currentStorage[0].entity = 0x30110u;
    previousStorage[0].assetSet = currentStorage[0].assetSet =
        RAGE_RENDER_ASSET_COURSE;
    previousStorage[0].assetKey = currentStorage[0].assetKey = 88;
    previousStorage[0].mesh = currentStorage[0].mesh = 63;
    previousStorage[0].transform.position.x = 10.0f;
    currentStorage[0].transform.position.x = 30.0f;
    currentStorage[0].previousTransform = currentStorage[0].transform;
    previous.instanceCount = current.instanceCount = 1;

    EXPECT_EQ(1, RageRenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.5f, presentation, 1));
    EXPECT_EQ(20, (int)presentation[0].transform.position.x);
}

static void test_native_camera_projection_has_no_gte_quantization(void) {
    RageRenderCamera camera = {0};
    RageRenderVec3 world = {10.0f, 5.0f, -100.0f};
    RageRenderVec3 view;
    RageRenderVec3 clip;
    float depthScale;
    float depthOffset;

    camera.verticalFovDegrees = 90.0f;
    camera.nearPlane = 1.0f;
    camera.farPlane = 1000.0f;
    RageRenderWorldToView(&camera, &world, &view);
    EXPECT_EQ(10, (int)view.x);
    EXPECT_EQ(-100, (int)view.z);
    EXPECT_EQ(1, RageRenderProject(&camera, &view, 2.0f, &clip));
    EXPECT_EQ(5, (int)(clip.x * 100.0f));
    EXPECT_EQ(5, (int)(clip.y * 100.0f));
    EXPECT_EQ(1, RageRenderPerspectiveDepthTerms(
                     &camera, &depthScale, &depthOffset));
    /* The GPU must receive a projective clip-space Z. A quadratic per-vertex
     * value makes a near terrain triangle intersect the camera planes at the
     * wrong place when one of its vertices is behind the cockpit camera. */
    EXPECT_EQ(0, (int)((camera.nearPlane * depthScale + depthOffset) * 1000.0f));
    EXPECT_EQ(1000, (int)(((camera.farPlane * depthScale + depthOffset) /
                           camera.farPlane) * 1000.0f));
    EXPECT_EQ(1, (-depthScale + depthOffset) < 0.0f);
    camera.transform.rotation.y = -90.0f;
    world.x = 100.0f; world.z = 0.0f;
    RageRenderWorldToView(&camera, &world, &view);
    EXPECT_EQ(-100, (int)view.z);
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

static void test_directional_shadow_map_is_texel_stable(void) {
    RageRenderVec3 light = RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;
    RageRenderVec3 center = {1000.0f, 200.0f, -500.0f};
    RageRenderVec3 moved = center;
    RageRenderVec3 projected;
    RageRenderShadowMap first;
    RageRenderShadowMap second;
    float firstRight;
    float secondRight;

    EXPECT_EQ(1, RageRenderBuildDirectionalShadowMap(
                     &center, &light, 4096.0f, 2048, &first));
    moved.x += first.texelWorldSize * 0.1f;
    EXPECT_EQ(1, RageRenderBuildDirectionalShadowMap(
                     &moved, &light, 4096.0f, 2048, &second));
    firstRight = first.position.x * first.row0.x +
                 first.position.y * first.row0.y +
                 first.position.z * first.row0.z;
    secondRight = second.position.x * second.row0.x +
                  second.position.y * second.row0.y +
                  second.position.z * second.row0.z;
    EXPECT_EQ(1, firstRight - secondRight < 0.01f &&
                 firstRight - secondRight > -0.01f);
    RageRenderProjectShadowPoint(&first, &center, &projected);
    EXPECT_EQ(0, (int)(projected.x * 100.0f));
    EXPECT_EQ(0, (int)(projected.y * 100.0f));
    EXPECT_EQ(50, (int)(projected.z * 100.0f));
}

static void test_default_shadow_light_stays_near_overhead(void) {
    const RageRenderVec3 light = RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;
    float horizontalSquared = light.x * light.x + light.z * light.z;
    /* Less than 14 degrees from vertical keeps a 100-unit-high caster's
     * shadow within 25 world units of its contact point. */
    EXPECT_EQ(1, light.y > 0.0f);
    EXPECT_EQ(1, horizontalSquared * 16.0f < light.y * light.y);
}

int main(void) {
    test_frame_reset_preserves_storage_and_resets_overflow();
    test_legacy_mirror_instances_can_be_removed_from_scene();
    test_camera_is_scene_data_not_backend_state();
    test_mirror_is_an_independent_scene_camera();
    test_perspective_fog_uses_authored_near_and_far_depths();
    test_terrain_grid_places_adjacent_cells_without_overlap();
    test_presentation_interpolates_without_mutating_game_world();
    test_synchronized_presentation_keeps_previous_vehicle_models();
    test_synchronized_presentation_moves_matching_vehicle();
    test_synchronized_presentation_keeps_wheel_sides_paired();
    test_synchronized_presentation_matches_animated_wheel_mesh();
    test_synchronized_presentation_moves_dynamic_scenery();
    test_native_camera_projection_has_no_gte_quantization();
    test_default_shadow_light_stays_near_overhead();
    test_psx_rotation_uses_the_same_basis_as_imported_positions();
    test_directional_shadow_map_is_texel_stable();
    if (failures != 0) return EXIT_FAILURE;
    puts("render world tests passed");
    return EXIT_SUCCESS;
}
