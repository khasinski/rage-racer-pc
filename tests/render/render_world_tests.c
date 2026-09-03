#include <math.h>
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

    RenderWorldInit(&world, storage, 1);
    RenderWorldBeginFrame(&world, 41);
    instance.entity = 7;
    instance.mesh = 12;
    instance.assetSet = RAGE_RENDER_ASSET_COURSE;
    instance.assetKey = 0x101;
    instance.material = 3;
    instance.pass = RAGE_RENDER_PASS_MIRROR;
    instance.transform.position.x = 22.0f;
    instance.previousTransform.position.x = 20.0f;
    EXPECT_EQ(1, RenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(0, RenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(1, world.instanceCount);
    EXPECT_EQ(1, world.overflowCount);
    EXPECT_EQ(7, storage[0].entity);
    EXPECT_EQ(RAGE_RENDER_ASSET_COURSE, storage[0].assetSet);
    EXPECT_EQ(0x101, storage[0].assetKey);
    EXPECT_EQ(RAGE_RENDER_PASS_MIRROR, storage[0].pass);

    RenderWorldBeginFrame(&world, 42);
    EXPECT_EQ(42, world.frame);
    EXPECT_EQ(0, world.instanceCount);
    EXPECT_EQ(0, world.overflowCount);
    EXPECT_EQ(1, RenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(20, (int)storage[0].previousTransform.position.x);
}

static void test_mesh_submission_rejects_invalid_storage(void) {
    RageRenderMeshInstance storage[1] = {0};
    RageRenderMeshInstance instance = {0};
    RageRenderWorld world;

    RenderWorldInit(&world, NULL, 1);
    EXPECT_EQ(0, RenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(1, world.overflowCount);

    RenderWorldInit(&world, storage, 1);
    world.instanceCount = 2;
    EXPECT_EQ(0, RenderWorldSubmitMesh(&world, &instance));
    EXPECT_EQ(1, world.overflowCount);
    EXPECT_EQ(0, RenderWorldSubmitMesh(NULL, &instance));
    EXPECT_EQ(0, RenderWorldSubmitMesh(&world, NULL));
}

static void test_public_world_mutators_reject_null_inputs(void) {
    RageRenderWorld world;
    RageRenderCamera camera = {0};

    RenderWorldInit(NULL, NULL, 0);
    RenderWorldInit(&world, NULL, 0);
    RenderWorldBeginFrame(NULL, 1);
    RenderWorldSetCamera(NULL, &camera);
    RenderWorldSetCamera(&world, NULL);
    RenderWorldSetMirrorCamera(NULL, &camera, 1, 0.0f);
    RenderWorldSetMirrorCamera(&world, NULL, 1, 0.0f);
    RenderTerrainCellTransform(0, 0, NULL);
    RenderConvertPsxMatrix(NULL, NULL);
    EXPECT_EQ(0, world.hasCamera);
    EXPECT_EQ(0, world.hasMirrorCamera);
}

static void test_legacy_mirror_instances_can_be_removed_from_scene(void) {
    RageRenderMeshInstance storage[3] = {0};
    RageRenderWorld world;

    RenderWorldInit(&world, storage, 3);
    storage[0].entity = 10;
    storage[0].pass = RAGE_RENDER_PASS_MAIN;
    storage[1].entity = 20;
    storage[1].pass = RAGE_RENDER_PASS_MIRROR;
    storage[2].entity = 30;
    storage[2].pass = RAGE_RENDER_PASS_MAIN;
    world.instanceCount = 3;
    RenderWorldDiscardPass(&world, RAGE_RENDER_PASS_MIRROR);
    EXPECT_EQ(2, world.instanceCount);
    EXPECT_EQ(10, storage[0].entity);
    EXPECT_EQ(30, storage[1].entity);
}

static void test_camera_is_scene_data_not_backend_state(void) {
    RageRenderMeshInstance storage[2];
    RageRenderWorld world;
    RageRenderCamera camera = {0};

    RenderWorldInit(&world, storage, 2);
    camera.transform.position.y = 3.5f;
    camera.verticalFovDegrees = 70.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 5000.0f;
    camera.fogColor.x = 0.25f;
    camera.skyTopColor.x = 0.10f;
    camera.skyColor.x = 0.20f;
    camera.skyHorizonColor.x = 0.30f;
    camera.skyBottomColor.x = 0.40f;
    camera.skyAssetKey = 88;
    camera.skyCloudRow = 2;
    camera.fogNear = 100.0f;
    camera.fogFar = 500.0f;
    RenderWorldSetCamera(&world, &camera);
    EXPECT_EQ(35, (int)(world.camera.transform.position.y * 10.0f));
    EXPECT_EQ(70, (int)world.camera.verticalFovDegrees);
    EXPECT_EQ(5000, (int)world.camera.farPlane);
    EXPECT_EQ(25, (int)(world.camera.fogColor.x * 100.0f));
    EXPECT_EQ(10, (int)(world.camera.skyTopColor.x * 100.0f));
    EXPECT_EQ(20, (int)(world.camera.skyColor.x * 100.0f));
    EXPECT_EQ(30, (int)(world.camera.skyHorizonColor.x * 100.0f));
    EXPECT_EQ(40, (int)(world.camera.skyBottomColor.x * 100.0f));
    EXPECT_EQ(88, world.camera.skyAssetKey);
    EXPECT_EQ(500, (int)world.camera.fogFar);

    RenderWorldBeginFrame(&world, 2);
    camera.transform.position.y = 13.5f;
    camera.verticalFovDegrees = 80.0f;
    camera.fogColor.x = 0.75f;
    camera.skyTopColor.x = 0.70f;
    camera.skyColor.x = 0.80f;
    camera.skyHorizonColor.x = 0.90f;
    camera.skyBottomColor.x = 1.00f;
    camera.skyAssetKey = 90;
    camera.skyCloudRow = 4;
    camera.fogNear = 200.0f;
    camera.fogFar = 1000.0f;
    RenderWorldSetCamera(&world, &camera);
    RenderInterpolateCamera(&world.previousCamera, &world.camera, 0.5f,
                                &camera);
    EXPECT_EQ(85, (int)(camera.transform.position.y * 10.0f));
    EXPECT_EQ(75, (int)camera.verticalFovDegrees);
    EXPECT_EQ(50, (int)(camera.fogColor.x * 100.0f));
    EXPECT_EQ(39, (int)(camera.skyTopColor.x * 100.0f));
    EXPECT_EQ(50, (int)(camera.skyColor.x * 100.0f));
    EXPECT_EQ(60, (int)(camera.skyHorizonColor.x * 100.0f));
    EXPECT_EQ(70, (int)(camera.skyBottomColor.x * 100.0f));
    EXPECT_EQ(90, camera.skyAssetKey);
    EXPECT_EQ(4, camera.skyCloudRow);
    EXPECT_EQ(150, (int)camera.fogNear);
    EXPECT_EQ(750, (int)camera.fogFar);
}

static void test_directional_light_is_scene_data(void) {
    RageRenderMeshInstance storage[1];
    RageRenderWorld world;
    RageRenderDirectionalLight light;

    RenderWorldInit(&world, storage, 1);
    EXPECT_EQ(35, (int)(world.light.ambientColor.x * 100.0f));
    EXPECT_EQ(65, (int)(world.light.diffuseColor.x * 100.0f));
    light.direction = (RageRenderVec3){1.0f, 2.0f, 3.0f};
    light.ambientColor = (RageRenderVec3){0.2f, 0.3f, 0.4f};
    light.diffuseColor = (RageRenderVec3){0.8f, 0.7f, 0.6f};
    RenderWorldSetDirectionalLight(&world, &light);
    EXPECT_EQ(2, (int)world.light.direction.y);
    EXPECT_EQ(30, (int)(world.light.ambientColor.y * 100.0f));
    EXPECT_EQ(60, (int)(world.light.diffuseColor.z * 100.0f));
}

static void test_mirror_is_an_independent_scene_camera(void) {
    RageRenderMeshInstance storage[1];
    RageRenderWorld world;
    RageRenderCamera camera = {0};

    RenderWorldInit(&world, storage, 1);
    camera.transform.position.z = 100.0f;
    camera.transform.rotation.y = 180.0f;
    camera.verticalFovDegrees = 20.0f;
    RenderWorldSetMirrorCamera(&world, &camera, 1, -12.0f);
    EXPECT_EQ(1, world.hasMirrorCamera);
    EXPECT_EQ(1, world.mirrorActive);
    EXPECT_EQ(180, (int)world.mirrorCamera.transform.rotation.y);
    EXPECT_EQ(20, (int)world.mirrorCamera.verticalFovDegrees);
    EXPECT_EQ(-12, (int)world.mirrorPanelY);

    RenderWorldBeginFrame(&world, 2);
    camera.transform.position.z = 140.0f;
    RenderWorldSetMirrorCamera(&world, &camera, 0, 18.0f);
    RenderInterpolateCamera(&world.previousMirrorCamera,
                                &world.mirrorCamera, 0.5f, &camera);
    EXPECT_EQ(120, (int)camera.transform.position.z);
    EXPECT_EQ(0, world.mirrorActive);
    EXPECT_EQ(-12, (int)world.previousMirrorPanelY);
    EXPECT_EQ(18, (int)world.mirrorPanelY);
}

static void test_camera_cuts_are_not_interpolated_as_motion(void) {
    RageRenderMeshInstance storage[1];
    RageRenderWorld world;
    RageRenderCamera camera = {0};
    RageRenderCamera presentation;

    RenderWorldInit(&world, storage, 1);
    camera.transform.hasOrientation = 1;
    camera.transform.orientation.w = 1.0f;
    camera.verticalFovDegrees = 45.0f;
    RenderWorldSetCamera(&world, &camera);
    RenderWorldBeginFrame(&world, 2);
    camera.transform.position.x = 4096.0f;
    camera.transform.orientation.y = 0.7071068f;
    camera.transform.orientation.w = 0.7071068f;
    RenderWorldSetCamera(&world, &camera);
    RenderInterpolateCamera(&world.previousCamera, &world.camera, 0.1f,
                                &presentation);
    EXPECT_EQ(4096, (int)presentation.transform.position.x);
    EXPECT_EQ(70, (int)(presentation.transform.orientation.y * 100.0f));
    EXPECT_EQ(70, (int)(presentation.transform.orientation.w * 100.0f));
}

static void test_transform_interpolation_takes_short_angle_path(void) {
    RageRenderTransform previous = {0};
    RageRenderTransform current = {0};
    RageRenderTransform presentation;

    previous.rotation.y = 350.0f;
    current.rotation.y = 10.0f;
    RenderInterpolateTransform(&previous, &current, 0.5f, &presentation);
    EXPECT_EQ(360, (int)presentation.rotation.y);
    EXPECT_EQ(350, (int)previous.rotation.y);
    EXPECT_EQ(10, (int)current.rotation.y);
}

static void test_perspective_fog_uses_authored_near_and_far_depths(void) {
    RageRenderCamera camera = {0};
    RageRenderVec3 point = {0.0f, 0.0f, -10.0f};
    camera.fogNear = 10.0f;
    camera.fogFar = 50.0f;
    EXPECT_EQ(0, (int)(RenderFogFactor(&camera, &point) * 100.0f));
    point.z = -25.0f;
    EXPECT_EQ(75, (int)(RenderFogFactor(&camera, &point) * 100.0f));
    point.z = -50.0f;
    EXPECT_EQ(100, (int)(RenderFogFactor(&camera, &point) * 100.0f));
}

static void test_projection_rejects_non_finite_camera_data(void) {
    RageRenderCamera camera = {0};
    RageRenderVec3 view = {0.0f, 0.0f, -10.0f};
    RageRenderVec3 clip;
    float scale, offset;

    camera.verticalFovDegrees = 60.0f;
    camera.nearPlane = 1.0f;
    camera.farPlane = 100.0f;
    camera.fogNear = 1.0f;
    camera.fogFar = 100.0f;
    EXPECT_EQ(0, RenderProject(&camera, &view, NAN, &clip));
    camera.verticalFovDegrees = INFINITY;
    EXPECT_EQ(0, RenderProject(&camera, &view, 1.0f, &clip));
    camera.verticalFovDegrees = 60.0f;
    camera.farPlane = NAN;
    EXPECT_EQ(0, RenderPerspectiveDepthTerms(&camera, &scale, &offset));
    camera.farPlane = 100.0f;
    view.z = NAN;
    EXPECT_EQ(0, RenderProject(&camera, &view, 1.0f, &clip));
    view.z = -10.0f;
    camera.fogFar = INFINITY;
    EXPECT_EQ(0, (int)RenderFogFactor(&camera, &view));
}

static void test_terrain_grid_places_adjacent_cells_without_overlap(void) {
    RageRenderTransform left;
    RageRenderTransform right;
    RageRenderTransform south;

    RenderTerrainCellTransform(0, 0, &left);
    RenderTerrainCellTransform(1, 0, &right);
    RenderTerrainCellTransform(0, 1, &south);
    EXPECT_EQ(4096, (int)left.position.x);
    EXPECT_EQ(258048, (int)left.position.z);
    EXPECT_EQ(8192, (int)(right.position.x - left.position.x));
    EXPECT_EQ(-8192, (int)(south.position.z - left.position.z));
    EXPECT_EQ(1, (int)left.scale.x);
}

static void test_synchronized_presentation_keeps_previous_vehicle_models(void) {
    RageRenderMeshInstance previousStorage[3] = {0};
    RageRenderMeshInstance currentStorage[3] = {0};
    RageRenderMeshInstance presentation[4] = {0};
    RageRenderWorld previous;
    RageRenderWorld current;
    uint32_t count;

    RenderWorldInit(&previous, previousStorage, 3);
    RenderWorldInit(&current, currentStorage, 3);
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

    count = RenderWorldBuildSynchronizedPresentation(
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

    RenderWorldInit(&previous, previousStorage, 1);
    RenderWorldInit(&current, currentStorage, 1);
    previousStorage[0].entity = currentStorage[0].entity = 7;
    previousStorage[0].mesh = currentStorage[0].mesh = 25;
    previousStorage[0].assetSet = currentStorage[0].assetSet =
        RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    previousStorage[0].assetKey = currentStorage[0].assetKey = 88;
    previousStorage[0].materialVariant = currentStorage[0].materialVariant = 28;
    previousStorage[0].transform.position.z = 100.0f;
    currentStorage[0].transform.position.z = 140.0f;
    previous.instanceCount = current.instanceCount = 1;

    EXPECT_EQ(1, RenderWorldBuildSynchronizedPresentation(
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

    RenderWorldInit(&previous, previousStorage, 2);
    RenderWorldInit(&current, currentStorage, 2);
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

    EXPECT_EQ(2, RenderWorldBuildSynchronizedPresentation(
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

    RenderWorldInit(&previous, previousStorage, 1);
    RenderWorldInit(&current, currentStorage, 1);
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

    EXPECT_EQ(1, RenderWorldBuildSynchronizedPresentation(
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

    RenderWorldInit(&previous, previousStorage, 1);
    RenderWorldInit(&current, currentStorage, 1);
    previousStorage[0].entity = currentStorage[0].entity = 0x30110u;
    previousStorage[0].assetSet = currentStorage[0].assetSet =
        RAGE_RENDER_ASSET_COURSE;
    previousStorage[0].assetKey = currentStorage[0].assetKey = 88;
    previousStorage[0].mesh = currentStorage[0].mesh = 63;
    previousStorage[0].transform.position.x = 10.0f;
    currentStorage[0].transform.position.x = 30.0f;
    currentStorage[0].previousTransform = currentStorage[0].transform;
    previous.instanceCount = current.instanceCount = 1;

    EXPECT_EQ(1, RenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.5f, presentation, 1));
    EXPECT_EQ(20, (int)presentation[0].transform.position.x);
}

static void test_synchronized_presentation_rejects_invalid_world_bounds(void) {
    RageRenderMeshInstance instance = {0};
    RageRenderMeshInstance presentation;
    RageRenderWorld previous;
    RageRenderWorld current;

    RenderWorldInit(&previous, &instance, 1);
    RenderWorldInit(&current, &instance, 1);
    previous.instanceCount = 2;
    EXPECT_EQ(0, RenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.5f, &presentation, 1));

    previous.instanceCount = 0;
    current.instanceCapacity = RAGE_RENDER_PRESENTATION_MAX_INSTANCES + 1;
    current.instanceCount = RAGE_RENDER_PRESENTATION_MAX_INSTANCES + 1;
    EXPECT_EQ(0, RenderWorldBuildSynchronizedPresentation(
                     &previous, &current, 0.5f, &presentation, 1));
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
    RenderWorldToView(&camera, &world, &view);
    EXPECT_EQ(10, (int)view.x);
    EXPECT_EQ(-100, (int)view.z);
    EXPECT_EQ(1, RenderProject(&camera, &view, 2.0f, &clip));
    EXPECT_EQ(5, (int)(clip.x * 100.0f));
    EXPECT_EQ(5, (int)(clip.y * 100.0f));
    EXPECT_EQ(1, RenderPerspectiveDepthTerms(
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
    RenderWorldToView(&camera, &world, &view);
    EXPECT_EQ(-100, (int)view.z);
}

static void test_psx_rotation_uses_the_same_basis_as_imported_positions(void) {
    const float source[3][3] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
    };
    float converted[3][3];
    RenderConvertPsxMatrix(source, converted);
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

    EXPECT_EQ(1, RenderBuildDirectionalShadowMap(
                     &center, &light, 4096.0f, 2048, &first));
    moved.x += first.texelWorldSize * 0.1f;
    EXPECT_EQ(1, RenderBuildDirectionalShadowMap(
                     &moved, &light, 4096.0f, 2048, &second));
    firstRight = first.position.x * first.row0.x +
                 first.position.y * first.row0.y +
                 first.position.z * first.row0.z;
    secondRight = second.position.x * second.row0.x +
                  second.position.y * second.row0.y +
                  second.position.z * second.row0.z;
    EXPECT_EQ(1, firstRight - secondRight < 0.01f &&
                 firstRight - secondRight > -0.01f);
    RenderProjectShadowPoint(&first, &center, &projected);
    EXPECT_EQ(0, (int)(projected.x * 100.0f));
    EXPECT_EQ(0, (int)(projected.y * 100.0f));
    EXPECT_EQ(50, (int)(projected.z * 100.0f));
}

static void test_high_resolution_vehicle_shadow_density(void) {
    RageRenderVec3 light = RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;
    RageRenderVec3 center = {0.0f, 0.0f, 0.0f};
    RageRenderShadowMap shadow;

    EXPECT_EQ(1, RenderBuildDirectionalShadowMap(
                     &center, &light, RAGE_RENDER_VEHICLE_SHADOW_EXTENT,
                     RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION, &shadow));
    EXPECT_EQ(200, (int)(shadow.texelWorldSize * 100.0f));
}

static void test_shadow_map_rejects_non_finite_geometry(void) {
    RageRenderVec3 center = {0.0f, 0.0f, 0.0f};
    RageRenderVec3 light = RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;
    RageRenderShadowMap shadow;

    center.x = INFINITY;
    EXPECT_EQ(0, RenderBuildDirectionalShadowMap(
                     &center, &light, 4096.0f, 2048, &shadow));
    center.x = 0.0f;
    light.y = NAN;
    EXPECT_EQ(0, RenderBuildDirectionalShadowMap(
                     &center, &light, 4096.0f, 2048, &shadow));
    light = RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;
    EXPECT_EQ(0, RenderBuildDirectionalShadowMap(
                     &center, &light, INFINITY, 2048, &shadow));
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
    test_mesh_submission_rejects_invalid_storage();
    test_public_world_mutators_reject_null_inputs();
    test_legacy_mirror_instances_can_be_removed_from_scene();
    test_camera_is_scene_data_not_backend_state();
    test_directional_light_is_scene_data();
    test_mirror_is_an_independent_scene_camera();
    test_camera_cuts_are_not_interpolated_as_motion();
    test_transform_interpolation_takes_short_angle_path();
    test_perspective_fog_uses_authored_near_and_far_depths();
    test_projection_rejects_non_finite_camera_data();
    test_terrain_grid_places_adjacent_cells_without_overlap();
    test_synchronized_presentation_keeps_previous_vehicle_models();
    test_synchronized_presentation_moves_matching_vehicle();
    test_synchronized_presentation_keeps_wheel_sides_paired();
    test_synchronized_presentation_matches_animated_wheel_mesh();
    test_synchronized_presentation_moves_dynamic_scenery();
    test_synchronized_presentation_rejects_invalid_world_bounds();
    test_native_camera_projection_has_no_gte_quantization();
    test_default_shadow_light_stays_near_overhead();
    test_psx_rotation_uses_the_same_basis_as_imported_positions();
    test_directional_shadow_map_is_texel_stable();
    test_high_resolution_vehicle_shadow_density();
    test_shadow_map_rejects_non_finite_geometry();
    if (failures != 0) return EXIT_FAILURE;
    puts("render world tests passed");
    return EXIT_SUCCESS;
}
