#include "car_parts.h"
#include "game/integer.h"
#include <string.h>

int BuildCarParts(const GameCarRuntime *car, const CarShape *shape,
                  s32 steering, CarPart parts[CAR_PART_COUNT]) {
    if (car == NULL || shape == NULL || parts == NULL) return 0;
    SceneMat3 base = SceneMat3Multiply(
        SceneRotationY(WrapSigned32((int64_t)ANGLE_HALF_TURN - car->bodyYaw)),
        SceneRotationX(car->bodyPitch));
    SceneMat3 wheels = SceneMat3Multiply(base, SceneRotationZ(
        WrapSigned32((int64_t)car->bodyRoll - car->bodyRollVelocity)));
    CarPart result[CAR_PART_COUNT];
    Vec3 origin = {(float)car->x,
        (float)((int64_t)RenderClampCarToGround(car->y, car->modelY) - shape->horizon),
        (float)car->z};
    result[0] = (CarPart){origin,
        SceneMat3Multiply(base, SceneRotationZ(car->bodyRoll))};
    result[1] = (CarPart){origin,
        SceneMat3Multiply(wheels, SceneRotationX(car->wheelRotation))};
    SceneMat3 front = SceneMat3Multiply(
        SceneMat3Multiply(wheels, SceneRotationY(steering)),
        SceneRotationX(car->wheelRotation));
    for (unsigned side = 0; side < 2; side++) {
        Vec3 position = SceneRotatePoint(wheels,
            side == 0 ? (float)shape->offsetX : -(float)shape->offsetX,
            (float)shape->offsetY, (float)shape->offsetZ);
        position.x += origin.x;
        position.y += origin.y;
        position.z += origin.z;
        result[side + 2] = (CarPart){position,
            side == 0 ? front : SceneMat3Multiply(front, SceneRotationY(ANGLE_HALF_TURN))};
    }
    memcpy(parts, result, sizeof(result));
    return 1;
}

RenderTransform CarPartTransform(const CarPart *part) {
    RenderTransform result = {0};
    if (!part) return result;
    result.position = (Vec3){part->position.x, -part->position.y, -part->position.z};
    /* Retail vertices use four units per game-world unit. */
    result.scale = (Vec3){0.25f, 0.25f, 0.25f};
    result.orientation = SceneQuaternionFromPsx(part->rotation);
    result.hasOrientation = 1;
    return result;
}

int BuildCarInstances(const GameCarRuntime *car, const GameCarRuntime *previous,
                      const CarShape *shape, const RenderMeshInstance *body,
                      u32 rearWheel, u32 frontWheel,
                      RenderMeshInstance out[CAR_PART_COUNT]) {
    if (!car || !previous || !shape || !body || !out ||
        (body->assetSet != RAGE_RENDER_ASSET_MODEL_BANK &&
         body->assetSet != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)) return 0;
    const int human = body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK;
    const s32 steering = human ? car->steeringAngle / 12
        : WrapSigned32((int64_t)car->steeringAngle * 2);
    const s32 oldSteering = human ? previous->steeringAngle / 12
        : WrapSigned32((int64_t)previous->steeringAngle * 2);
    CarPart parts[CAR_PART_COUNT], oldParts[CAR_PART_COUNT];
    if (!BuildCarParts(car, shape, steering, parts) ||
        !BuildCarParts(previous, shape, oldSteering, oldParts)) return 0;
    const u32 meshes[CAR_PART_COUNT] = {body->mesh, rearWheel, frontWheel, frontWheel};
    const u8 components[CAR_PART_COUNT] = {0, 2, 3, 4};
    RenderMeshInstance result[CAR_PART_COUNT];
    for (unsigned part = 0; part < CAR_PART_COUNT; ++part) {
        result[part] = *body;
        result[part].mesh = meshes[part];
        result[part].component = components[part];
        result[part].transform = CarPartTransform(&parts[part]);
        result[part].previousTransform = CarPartTransform(&oldParts[part]);
        if (part) {
            result[part].materialVariant = human ? 0 : body->materialVariant / 3u * 3u;
            result[part].lamps = (CarLights){0};
        }
    }
    memcpy(out, result, sizeof(result));
    return 1;
}
