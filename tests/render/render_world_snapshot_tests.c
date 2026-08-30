#include "render/render_world_snapshot.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) do {                                               \
    if (!(condition)) {                                                     \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                #condition);                                                \
        failures++;                                                         \
    }                                                                       \
} while (0)

static void FillTransform(RageRenderTransform *transform, float base) {
    transform->position = (RageRenderVec3){base + 1, base + 2, base + 3};
    transform->rotation = (RageRenderVec3){base + 4, base + 5, base + 6};
    transform->scale = (RageRenderVec3){base + 7, base + 8, base + 9};
    transform->orientation = (RageRenderQuaternion){
        base + 10, base + 11, base + 12, base + 13};
    transform->hasOrientation = 1;
}

static void FillCamera(RageRenderCamera *camera, float base) {
    FillTransform(&camera->transform, base);
    camera->verticalFovDegrees = base + 20;
    camera->nearPlane = base + 21;
    camera->farPlane = base + 22;
    camera->fogColor = (RageRenderVec3){base + 23, base + 24, base + 25};
    camera->skyTopColor = (RageRenderVec3){base + 26, base + 27, base + 28};
    camera->skyColor = (RageRenderVec3){base + 29, base + 30, base + 31};
    camera->skyHorizonColor =
        (RageRenderVec3){base + 32, base + 33, base + 34};
    camera->skyBottomColor =
        (RageRenderVec3){base + 35, base + 36, base + 37};
    camera->skyAssetKey = (uint32_t)(base + 38);
    camera->fogNear = base + 39;
    camera->fogFar = base + 40;
}

static int SameTransform(const RageRenderTransform *a,
                         const RageRenderTransform *b) {
    return memcmp(&a->position, &b->position, sizeof(a->position)) == 0 &&
           memcmp(&a->rotation, &b->rotation, sizeof(a->rotation)) == 0 &&
           memcmp(&a->scale, &b->scale, sizeof(a->scale)) == 0 &&
           memcmp(&a->orientation, &b->orientation,
                  sizeof(a->orientation)) == 0 &&
           a->hasOrientation == b->hasOrientation;
}

static int SameCamera(const RageRenderCamera *a, const RageRenderCamera *b) {
    return SameTransform(&a->transform, &b->transform) &&
           a->verticalFovDegrees == b->verticalFovDegrees &&
           a->nearPlane == b->nearPlane && a->farPlane == b->farPlane &&
           memcmp(&a->fogColor, &b->fogColor, sizeof(a->fogColor)) == 0 &&
           memcmp(&a->skyTopColor, &b->skyTopColor,
                  sizeof(a->skyTopColor)) == 0 &&
           memcmp(&a->skyColor, &b->skyColor, sizeof(a->skyColor)) == 0 &&
           memcmp(&a->skyHorizonColor, &b->skyHorizonColor,
                  sizeof(a->skyHorizonColor)) == 0 &&
           memcmp(&a->skyBottomColor, &b->skyBottomColor,
                  sizeof(a->skyBottomColor)) == 0 &&
           a->skyAssetKey == b->skyAssetKey &&
           a->fogNear == b->fogNear && a->fogFar == b->fogFar;
}

static int SameLight(const RageRenderDirectionalLight *a,
                     const RageRenderDirectionalLight *b) {
    return memcmp(a, b, sizeof(*a)) == 0;
}

static void TestRoundTrip(void) {
    const char *path = "render-world-snapshot-test.bin";
    RageRenderMeshInstance instances[2] = {0};
    RageRenderWorld world = {0};
    RageRenderWorldSnapshot loaded;
    world.frame = UINT64_C(0x123456789abcdef0);
    world.light.direction = (RageRenderVec3){.1f, .2f, .3f};
    world.light.ambientColor = (RageRenderVec3){.4f, .5f, .6f};
    world.light.diffuseColor = (RageRenderVec3){.7f, .8f, .9f};
    FillCamera(&world.camera, 1);
    FillCamera(&world.previousCamera, 40);
    FillCamera(&world.mirrorCamera, 80);
    FillCamera(&world.previousMirrorCamera, 120);
    world.hasCamera = 1;
    world.hasMirrorCamera = 1;
    world.mirrorActive = 1;
    world.mirrorPanelY = -36.5f;
    world.previousMirrorPanelY = -37.5f;
    world.instances = instances;
    world.instanceCapacity = 2;
    world.instanceCount = 2;
    world.overflowCount = 3;
    instances[0].entity = 11;
    instances[0].mesh = 12;
    instances[0].assetSet = RAGE_RENDER_ASSET_TERRAIN;
    instances[0].assetKey = 13;
    instances[0].material = 14;
    instances[0].component = 15;
    instances[0].materialVariant = 16;
    instances[0].hasCarPaint = 1;
    instances[0].carPaintColor1 = 17;
    instances[0].carPaintColor2 = 18;
    instances[0].textureScrollU = 19;
    instances[0].lightInfluence = 0.4f;
    instances[0].environmentLight = (RageRenderVec3){.1f, .2f, .3f};
    instances[0].depthBias = -2.5f;
    FillTransform(&instances[0].transform, 160);
    FillTransform(&instances[0].previousTransform, 180);
    instances[0].flags = RAGE_RENDER_INSTANCE_ENABLE_FOG |
                         RAGE_RENDER_INSTANCE_DEPTH_DECAL;
    instances[0].pass = RAGE_RENDER_PASS_MIRROR;
    instances[1] = instances[0];
    instances[1].entity = 21;
    instances[1].assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2;
    instances[1].pass = RAGE_RENDER_PASS_MAIN;

    CHECK(RenderWorldSnapshotWrite(path, &world));
    CHECK(RenderWorldSnapshotRead(path, &loaded));
    CHECK(loaded.world.frame == world.frame);
    CHECK(SameLight(&loaded.world.light, &world.light));
    CHECK(SameCamera(&loaded.world.camera, &world.camera));
    CHECK(SameCamera(&loaded.world.previousCamera, &world.previousCamera));
    CHECK(SameCamera(&loaded.world.mirrorCamera, &world.mirrorCamera));
    CHECK(SameCamera(&loaded.world.previousMirrorCamera,
                     &world.previousMirrorCamera));
    CHECK(loaded.world.instanceCount == 2);
    CHECK(loaded.world.instanceCapacity == 2);
    CHECK(loaded.world.overflowCount == 3);
    CHECK(loaded.instances == loaded.world.instances);
    CHECK(loaded.instances[0].entity == 11);
    CHECK(loaded.instances[0].assetSet == RAGE_RENDER_ASSET_TERRAIN);
    CHECK(loaded.instances[0].material == 14);
    CHECK(loaded.instances[0].lightInfluence == 0.4f);
    CHECK(loaded.instances[0].depthBias == -2.5f);
    CHECK(SameTransform(&loaded.instances[0].transform,
                        &instances[0].transform));
    CHECK(SameTransform(&loaded.instances[0].previousTransform,
                        &instances[0].previousTransform));
    CHECK(loaded.instances[0].flags == instances[0].flags);
    CHECK(loaded.instances[0].pass == RAGE_RENDER_PASS_MIRROR);
    CHECK(loaded.instances[1].entity == 21);
    CHECK(loaded.instances[1].assetSet ==
          RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2);
    RenderWorldSnapshotRelease(&loaded);
    remove(path);
}

static void TestRejectsInvalidFile(void) {
    const char *path = "render-world-snapshot-invalid.bin";
    RageRenderWorldSnapshot loaded;
    FILE *file = fopen(path, "wb");
    CHECK(file != NULL);
    if (file != NULL) {
        fwrite("not a frame", 1, 11, file);
        fclose(file);
    }
    CHECK(!RenderWorldSnapshotRead(path, &loaded));
    remove(path);
}

int main(void) {
    TestRoundTrip();
    TestRejectsInvalidFile();
    return failures == 0 ? 0 : 1;
}
