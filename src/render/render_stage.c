#include "render_stage.h"

#include <math.h>
#include <string.h>

static float Radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

void RenderStageDefaults(RageRenderStage *stage) {
    if (stage == NULL) return;
    memset(stage, 0, sizeof(*stage));
    /* A car is roughly a thousand world units long, so this frames one with
     * room around it and puts the eye a little above the roof. */
    stage->distance = 1200.0f;
    stage->elevationDegrees = 15.0f;
    stage->verticalFovDegrees = 30.0f;
    stage->nearPlane = 16.0f;
    stage->farPlane = 200000.0f;
}

static RageRenderQuaternion MultiplyQuaternion(RageRenderQuaternion a,
                                              RageRenderQuaternion b) {
    RageRenderQuaternion out;
    out.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    out.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    out.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    out.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return out;
}

static RageRenderQuaternion AxisQuaternion(int axis, float degrees) {
    float half = Radians(degrees) * 0.5f;
    float sine = sinf(half);
    RageRenderQuaternion out;
    out.x = axis == 0 ? sine : 0.0f;
    out.y = axis == 1 ? sine : 0.0f;
    out.z = axis == 2 ? sine : 0.0f;
    out.w = cosf(half);
    return out;
}

/* The same rotation the Euler triple describes, in the form the game uses.
 * The scene applies its Euler angles X first, then Y, then Z, so the
 * quaternion composes in the opposite order. */
static RageRenderQuaternion QuaternionFromEuler(const RageRenderVec3 *degrees) {
    return MultiplyQuaternion(
        AxisQuaternion(2, degrees->z),
        MultiplyQuaternion(AxisQuaternion(1, degrees->y),
                           AxisQuaternion(0, degrees->x)));
}

void RenderPoseDefaults(RageRenderPose *pose) {
    if (pose == NULL) return;
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
    if (camera == NULL) return;
    memset(camera, 0, sizeof(*camera));
    if (stage == NULL) return;

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
    if (world == NULL) return 0;
    if (stage == NULL ||
        (capacity != 0 && storage == NULL) ||
        (count != 0 && poses == NULL) ||
        (capacity != 0 && sizeof(*storage) > SIZE_MAX / capacity)) {
        RenderWorldInit(world, NULL, 0);
        return 0;
    }

    RenderWorldInit(world, storage, capacity);
    if (capacity != 0) memset(storage, 0, sizeof(*storage) * capacity);

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
        if (pose->useQuaternion) {
            instance->transform.orientation =
                QuaternionFromEuler(&pose->rotationDegrees);
            instance->transform.hasOrientation = 1;
        }
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
