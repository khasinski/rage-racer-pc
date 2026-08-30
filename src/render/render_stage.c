#include "render_stage.h"

#include <math.h>
#include <string.h>

static float Radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

void RenderStageDefaults(RageRenderStage *stage) {
    if (stage == 0) return;
    memset(stage, 0, sizeof(*stage));
    /* A car is roughly a thousand world units long, so this frames one with
     * room around it and puts the eye a little above the roof. */
    stage->distance = 1200.0f;
    stage->elevationDegrees = 15.0f;
    stage->verticalFovDegrees = 30.0f;
    stage->nearPlane = 16.0f;
    stage->farPlane = 200000.0f;
}

void RenderPoseDefaults(RageRenderPose *pose) {
    if (pose == 0) return;
    memset(pose, 0, sizeof(*pose));
    pose->assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    pose->flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING;
    pose->lightInfluence = 1.0f;
}

/*
 * The camera's own orientation is the orbit read as angles: elevation is its
 * pitch and azimuth its yaw. Its position is then one orbit radius back along
 * the direction it faces, which is -Z in camera space.
 */
void RenderStageCamera(const RageRenderStage *stage,
                       RageRenderCamera *camera) {
    float pitch, yaw, cosPitch;
    RageRenderVec3 forward;
    if (stage == 0 || camera == 0) return;
    memset(camera, 0, sizeof(*camera));

    /* Climbing above the subject means looking down at it, which is a
     * negative pitch. Elevation is stated the way a person means it. */
    pitch = Radians(-stage->elevationDegrees);
    yaw = Radians(stage->azimuthDegrees);
    cosPitch = cosf(pitch);
    forward.x = -cosPitch * sinf(yaw);
    forward.y = sinf(pitch);
    forward.z = -cosPitch * cosf(yaw);

    camera->transform.rotation.x = -stage->elevationDegrees;
    camera->transform.rotation.y = stage->azimuthDegrees;
    camera->transform.rotation.z = stage->rollDegrees;
    camera->transform.scale.x = 1.0f;
    camera->transform.scale.y = 1.0f;
    camera->transform.scale.z = 1.0f;
    camera->transform.position.x = stage->target.x - forward.x * stage->distance;
    camera->transform.position.y = stage->target.y - forward.y * stage->distance;
    camera->transform.position.z = stage->target.z - forward.z * stage->distance;

    camera->verticalFovDegrees = stage->verticalFovDegrees;
    camera->nearPlane = stage->nearPlane;
    camera->farPlane = stage->farPlane;
    /* Nothing on a stage is fogged: the point is to see the subject itself. */
    camera->fogNear = stage->farPlane;
    camera->fogFar = stage->farPlane;
}

uint32_t RenderStageCompose(RageRenderWorld *world,
                            RageRenderMeshInstance *storage,
                            uint32_t capacity,
                            const RageRenderStage *stage,
                            const RageRenderPose *poses, uint32_t count) {
    uint32_t placed = 0;
    uint32_t index;
    if (world == 0 || storage == 0 || stage == 0) return 0;

    memset(world, 0, sizeof(*world));
    memset(storage, 0, sizeof(*storage) * capacity);
    world->instances = storage;
    world->instanceCapacity = capacity;

    /* One light from over the camera's shoulder, so a subject turning on the
     * spot is lit the same way at every angle it is seen from. */
    world->light.direction.x = 0.3f;
    world->light.direction.y = -0.8f;
    world->light.direction.z = 0.5f;
    world->light.ambientColor.x = 0.35f;
    world->light.ambientColor.y = 0.35f;
    world->light.ambientColor.z = 0.38f;
    world->light.diffuseColor.x = 0.85f;
    world->light.diffuseColor.y = 0.85f;
    world->light.diffuseColor.z = 0.80f;

    RenderStageCamera(stage, &world->camera);
    world->previousCamera = world->camera;
    world->hasCamera = 1;

    for (index = 0; index < count && placed < capacity; index++) {
        RageRenderMeshInstance *instance = &storage[placed];
        const RageRenderPose *pose = &poses[index];
        instance->entity = placed + 1;
        instance->mesh = pose->mesh;
        instance->assetSet = pose->assetSet;
        instance->assetKey = pose->assetKey;
        instance->materialVariant = pose->materialVariant;
        instance->flags = pose->flags;
        instance->lightInfluence = pose->lightInfluence;
        instance->transform.position = pose->position;
        instance->transform.rotation = pose->rotationDegrees;
        instance->transform.scale.x = 1.0f;
        instance->transform.scale.y = 1.0f;
        instance->transform.scale.z = 1.0f;
        instance->environmentLight.x = 1.0f;
        instance->environmentLight.y = 1.0f;
        instance->environmentLight.z = 1.0f;
        placed++;
    }
    world->instanceCount = placed;
    world->overflowCount = count > placed ? count - placed : 0;
    return placed;
}
