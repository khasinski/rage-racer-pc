#include "render_world_frame.h"

#include <math.h>
#include <string.h>

enum { RAGE_RENDER_PRESENTATION_MATCH_CAPACITY = 4096 };

static float RageClamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

float RageRenderLerpAngleDegrees(float from, float to, float t) {
    float delta;
    t = RageClamp01(t);
    delta = to - from;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return from + delta * t;
}

void RageRenderInterpolateTransform(const RageRenderTransform *previous,
                                    const RageRenderTransform *current,
                                    float t,
                                    RageRenderTransform *out) {
    t = RageClamp01(t);
    out->position.x = previous->position.x +
                      (current->position.x - previous->position.x) * t;
    out->position.y = previous->position.y +
                      (current->position.y - previous->position.y) * t;
    out->position.z = previous->position.z +
                      (current->position.z - previous->position.z) * t;
    out->rotation.x = RageRenderLerpAngleDegrees(previous->rotation.x,
                                                  current->rotation.x, t);
    out->rotation.y = RageRenderLerpAngleDegrees(previous->rotation.y,
                                                  current->rotation.y, t);
    out->rotation.z = RageRenderLerpAngleDegrees(previous->rotation.z,
                                                  current->rotation.z, t);
    out->hasOrientation = previous->hasOrientation && current->hasOrientation;
    if (out->hasOrientation) {
        float dot = previous->orientation.x * current->orientation.x +
                    previous->orientation.y * current->orientation.y +
                    previous->orientation.z * current->orientation.z +
                    previous->orientation.w * current->orientation.w;
        float sign = dot < 0.0f ? -1.0f : 1.0f;
        float length;
        out->orientation.x = previous->orientation.x +
            (current->orientation.x * sign - previous->orientation.x) * t;
        out->orientation.y = previous->orientation.y +
            (current->orientation.y * sign - previous->orientation.y) * t;
        out->orientation.z = previous->orientation.z +
            (current->orientation.z * sign - previous->orientation.z) * t;
        out->orientation.w = previous->orientation.w +
            (current->orientation.w * sign - previous->orientation.w) * t;
        length = sqrtf(out->orientation.x * out->orientation.x +
                       out->orientation.y * out->orientation.y +
                       out->orientation.z * out->orientation.z +
                       out->orientation.w * out->orientation.w);
        if (length > 0.0f) {
            out->orientation.x /= length; out->orientation.y /= length;
            out->orientation.z /= length; out->orientation.w /= length;
        }
    } else {
        out->orientation = current->orientation;
    }
    out->scale.x = previous->scale.x +
                   (current->scale.x - previous->scale.x) * t;
    out->scale.y = previous->scale.y +
                   (current->scale.y - previous->scale.y) * t;
    out->scale.z = previous->scale.z +
                   (current->scale.z - previous->scale.z) * t;
}

void RageRenderInterpolateCamera(const RageRenderCamera *previous,
                                 const RageRenderCamera *current, float t,
                                 RageRenderCamera *out) {
    RageRenderInterpolateTransform(&previous->transform, &current->transform,
                                   t, &out->transform);
    t = RageClamp01(t);
    out->verticalFovDegrees = previous->verticalFovDegrees +
        (current->verticalFovDegrees - previous->verticalFovDegrees) * t;
    out->nearPlane = previous->nearPlane +
        (current->nearPlane - previous->nearPlane) * t;
    out->farPlane = previous->farPlane +
        (current->farPlane - previous->farPlane) * t;
    out->fogColor.x = previous->fogColor.x +
        (current->fogColor.x - previous->fogColor.x) * t;
    out->fogColor.y = previous->fogColor.y +
        (current->fogColor.y - previous->fogColor.y) * t;
    out->fogColor.z = previous->fogColor.z +
        (current->fogColor.z - previous->fogColor.z) * t;
    out->fogNear = previous->fogNear +
        (current->fogNear - previous->fogNear) * t;
    out->fogFar = previous->fogFar +
        (current->fogFar - previous->fogFar) * t;
}

uint32_t RageRenderWorldBuildPresentation(const RageRenderWorld *world,
                                          float t,
                                          RageRenderMeshInstance *out,
                                          uint32_t capacity) {
    uint32_t count;
    uint32_t index;

    if (world == 0 || out == 0) return 0;
    count = world->instanceCount < capacity ? world->instanceCount : capacity;
    for (index = 0; index < count; index++) {
        out[index] = world->instances[index];
        RageRenderInterpolateTransform(&world->instances[index].previousTransform,
                                       &world->instances[index].transform, t,
                                       &out[index].transform);
    }
    return count;
}

static int RageRenderInstanceIsVehicle(
    const RageRenderMeshInstance *instance) {
    return instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
           instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
}

static int RageRenderVehicleIdentityMatches(
    const RageRenderMeshInstance *left,
    const RageRenderMeshInstance *right) {
    return left->entity == right->entity &&
           left->mesh == right->mesh &&
           left->assetSet == right->assetSet &&
           left->assetKey == right->assetKey &&
           left->materialVariant == right->materialVariant &&
           left->pass == right->pass;
}

static float RageRenderTransformDistanceSquared(
    const RageRenderTransform *left, const RageRenderTransform *right) {
    float x = left->position.x - right->position.x;
    float y = left->position.y - right->position.y;
    float z = left->position.z - right->position.z;
    return x * x + y * y + z * z;
}

uint32_t RageRenderWorldBuildSynchronizedPresentation(
    const RageRenderWorld *previous, const RageRenderWorld *current, float t,
    RageRenderMeshInstance *out, uint32_t capacity) {
    uint32_t outputCount = 0;
    uint32_t currentIndex;
    uint8_t matched[RAGE_RENDER_PRESENTATION_MATCH_CAPACITY];

    if (previous == 0 || current == 0 || out == 0) return 0;
    memset(matched, 0, sizeof(matched));

    /* Complete course and terrain publication is stable across ticks. Keep
     * its existing producer-provided history without an expensive global
     * instance match. */
    for (currentIndex = 0;
         currentIndex < current->instanceCount && outputCount < capacity;
         currentIndex++) {
        const RageRenderMeshInstance *instance =
            &current->instances[currentIndex];
        if (RageRenderInstanceIsVehicle(instance)) continue;
        out[outputCount] = *instance;
        RageRenderInterpolateTransform(&instance->previousTransform,
                                       &instance->transform, t,
                                       &out[outputCount].transform);
        outputCount++;
    }

    /* ModernPresentSource composites against RageCapturePrevious(). Vehicles
     * must therefore come from the previous world's visibility/model set too.
     * Using the current list made fast GP-intro camera cuts display a different
     * rival (or no rival) while the rest of the frame was still one tick back. */
    for (uint32_t previousIndex = 0;
         previousIndex < previous->instanceCount && outputCount < capacity;
         previousIndex++) {
        const RageRenderMeshInstance *base =
            &previous->instances[previousIndex];
        const RageRenderMeshInstance *target = 0;
        uint32_t targetIndex = 0;
        float bestDistance = 0.0f;

        if (!RageRenderInstanceIsVehicle(base)) continue;
        for (currentIndex = 0; currentIndex < current->instanceCount;
             currentIndex++) {
            const RageRenderMeshInstance *candidate =
                &current->instances[currentIndex];
            float distance;
            if (currentIndex >= sizeof(matched) || matched[currentIndex] ||
                !RageRenderVehicleIdentityMatches(base, candidate))
                continue;
            distance = RageRenderTransformDistanceSquared(
                &base->transform, &candidate->transform);
            if (target == 0 || distance < bestDistance) {
                target = candidate;
                targetIndex = currentIndex;
                bestDistance = distance;
            }
        }
        out[outputCount] = *base;
        if (target != 0) {
            matched[targetIndex] = 1;
            RageRenderInterpolateTransform(&base->transform,
                                           &target->transform, t,
                                           &out[outputCount].transform);
        }
        outputCount++;
    }
    return outputCount;
}
