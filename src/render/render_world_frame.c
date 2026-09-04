#include "render_world_frame.h"

#include <math.h>
#include <string.h>

static int RenderWorldInstancesAreValid(const RageRenderWorld *world) {
    return world != NULL &&
           world->instanceCount <= world->instanceCapacity &&
           world->instanceCount <= RAGE_RENDER_PRESENTATION_MAX_INSTANCES &&
           (world->instanceCount == 0 || world->instances != NULL);
}

static float Clamp01(float value) {
    if (!isfinite(value)) return 0.0f;
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void InterpolateVec3(const RageRenderVec3 *previous,
                            const RageRenderVec3 *current, float t,
                            RageRenderVec3 *out) {
    out->x = previous->x + (current->x - previous->x) * t;
    out->y = previous->y + (current->y - previous->y) * t;
    out->z = previous->z + (current->z - previous->z) * t;
}

static float InterpolateWrapped(float previous, float current,
                                float period, float t) {
    float delta = current - previous;
    while (delta > period * 0.5f) delta -= period;
    while (delta < period * -0.5f) delta += period;
    return previous + delta * t;
}

float RenderLerpAngleDegrees(float from, float to, float t) {
    float delta;

    if (!isfinite(from) || !isfinite(to)) return 0.0f;
    t = Clamp01(t);
    delta = to - from;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return from + delta * t;
}

void RenderInterpolateTransform(const RageRenderTransform *previous,
                                    const RageRenderTransform *current,
                                    float t,
                                    RageRenderTransform *out) {
    if (out == NULL) return;
    if (previous == NULL || current == NULL) {
        memset(out, 0, sizeof(*out));
        return;
    }
    t = Clamp01(t);
    InterpolateVec3(&previous->position, &current->position, t,
                    &out->position);
    out->rotation.x = RenderLerpAngleDegrees(previous->rotation.x,
                                                  current->rotation.x, t);
    out->rotation.y = RenderLerpAngleDegrees(previous->rotation.y,
                                                  current->rotation.y, t);
    out->rotation.z = RenderLerpAngleDegrees(previous->rotation.z,
                                                  current->rotation.z, t);
    out->hasOrientation = previous->hasOrientation && current->hasOrientation;
    if (out->hasOrientation) {
        double dot =
            (double)previous->orientation.x * current->orientation.x +
            (double)previous->orientation.y * current->orientation.y +
            (double)previous->orientation.z * current->orientation.z +
            (double)previous->orientation.w * current->orientation.w;
        float sign = dot < 0.0 ? -1.0f : 1.0f;
        double lengthSquared;
        out->orientation.x = previous->orientation.x +
            (current->orientation.x * sign - previous->orientation.x) * t;
        out->orientation.y = previous->orientation.y +
            (current->orientation.y * sign - previous->orientation.y) * t;
        out->orientation.z = previous->orientation.z +
            (current->orientation.z * sign - previous->orientation.z) * t;
        out->orientation.w = previous->orientation.w +
            (current->orientation.w * sign - previous->orientation.w) * t;
        lengthSquared =
            (double)out->orientation.x * out->orientation.x +
            (double)out->orientation.y * out->orientation.y +
            (double)out->orientation.z * out->orientation.z +
            (double)out->orientation.w * out->orientation.w;
        if (isfinite(lengthSquared) && lengthSquared > 0.0) {
            double inverseLength = 1.0 / sqrt(lengthSquared);
            out->orientation.x =
                (float)((double)out->orientation.x * inverseLength);
            out->orientation.y =
                (float)((double)out->orientation.y * inverseLength);
            out->orientation.z =
                (float)((double)out->orientation.z * inverseLength);
            out->orientation.w =
                (float)((double)out->orientation.w * inverseLength);
        }
    } else {
        out->orientation = current->orientation;
    }
    InterpolateVec3(&previous->scale, &current->scale, t, &out->scale);
}

void RenderInterpolateCamera(const RageRenderCamera *previous,
                                 const RageRenderCamera *current, float t,
                                 RageRenderCamera *out) {
    if (out == NULL) return;
    if (previous == NULL || current == NULL) {
        memset(out, 0, sizeof(*out));
        return;
    }
    RenderInterpolateTransform(&previous->transform, &current->transform,
                                   t, &out->transform);
    t = Clamp01(t);
    out->verticalFovDegrees = previous->verticalFovDegrees +
        (current->verticalFovDegrees - previous->verticalFovDegrees) * t;
    out->nearPlane = previous->nearPlane +
        (current->nearPlane - previous->nearPlane) * t;
    out->farPlane = previous->farPlane +
        (current->farPlane - previous->farPlane) * t;
    InterpolateVec3(&previous->fogColor, &current->fogColor, t,
                    &out->fogColor);
    InterpolateVec3(&previous->skyTopColor, &current->skyTopColor, t,
                    &out->skyTopColor);
    InterpolateVec3(&previous->skyColor, &current->skyColor, t,
                    &out->skyColor);
    InterpolateVec3(&previous->skyHorizonColor,
                    &current->skyHorizonColor, t, &out->skyHorizonColor);
    InterpolateVec3(&previous->skyBottomColor, &current->skyBottomColor, t,
                    &out->skyBottomColor);
    out->skyAssetKey = current->skyAssetKey;
    /* Texture identities and authored sheet rows are discrete scene state;
     * both switch together instead of being numerically interpolated. */
    out->skyCloudRow = current->skyCloudRow;
    if (previous->skyCloudRow != current->skyCloudRow ||
        previous->skyAssetKey != current->skyAssetKey) {
        out->skyGridOrigin = current->skyGridOrigin;
        out->skyGridColumn = current->skyGridColumn;
        out->skyGridRow = current->skyGridRow;
    } else {
        InterpolateVec3(&previous->skyGridOrigin, &current->skyGridOrigin, t,
                        &out->skyGridOrigin);
        InterpolateVec3(&previous->skyGridColumn, &current->skyGridColumn, t,
                        &out->skyGridColumn);
        InterpolateVec3(&previous->skyGridRow, &current->skyGridRow, t,
                        &out->skyGridRow);
        /* The panorama is eight tiles wide, while the classic layout's
         * texture column wraps after the 32 half-angle steps.  Treating the
         * 31 -> 0 boundary as an ordinary scalar sends an interpolated frame
         * backwards through almost four panoramas, which looks like flashing
         * and counter-motion during rotating attract cameras. */
        out->skyGridColumn.z = InterpolateWrapped(
            previous->skyGridColumn.z, current->skyGridColumn.z, 8.0f, t);
    }
    out->fogNear = previous->fogNear +
        (current->fogNear - previous->fogNear) * t;
    out->fogFar = previous->fogFar +
        (current->fogFar - previous->fogFar) * t;
}

static int RenderInstanceIsVehicle(
    const RageRenderMeshInstance *instance) {
    return instance->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
           instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
}

static int RenderInstanceNeedsSynchronizedMatch(
    const RageRenderMeshInstance *instance) {
    return RenderInstanceIsVehicle(instance) ||
           (instance->assetSet == RAGE_RENDER_ASSET_COURSE &&
            instance->entity >= 0x30000u && instance->entity < 0x40000u);
}

static int RenderVehicleIdentityMatches(
    const RageRenderMeshInstance *left,
    const RageRenderMeshInstance *right) {
    if (left->entity != right->entity ||
        left->assetSet != right->assetSet ||
        left->assetKey != right->assetKey ||
        left->materialVariant != right->materialVariant ||
        left->pass != right->pass)
        return 0;
    if (RenderInstanceIsVehicle(left) &&
        RenderInstanceIsVehicle(right))
        return left->component == right->component;
    return left->mesh == right->mesh;
}

static float RenderTransformDistanceSquared(
    const RageRenderTransform *left, const RageRenderTransform *right) {
    float x = left->position.x - right->position.x;
    float y = left->position.y - right->position.y;
    float z = left->position.z - right->position.z;
    return x * x + y * y + z * z;
}

uint32_t RenderWorldBuildSynchronizedPresentation(
    const RageRenderWorld *previous, const RageRenderWorld *current, float t,
    RageRenderMeshInstance *out, uint32_t capacity) {
    uint32_t outputCount = 0;
    uint32_t currentIndex;
    uint8_t matched[RAGE_RENDER_PRESENTATION_MAX_INSTANCES];

    if (!RenderWorldInstancesAreValid(previous) ||
        !RenderWorldInstancesAreValid(current) || out == NULL) return 0;
    memset(matched, 0, sizeof(matched));

    /* Complete course and terrain publication is stable across ticks. Keep
     * its existing producer-provided history without an expensive global
     * instance match. */
    for (currentIndex = 0;
         currentIndex < current->instanceCount && outputCount < capacity;
         currentIndex++) {
        const RageRenderMeshInstance *instance =
            &current->instances[currentIndex];
        if (RenderInstanceNeedsSynchronizedMatch(instance)) continue;
        out[outputCount] = *instance;
        RenderInterpolateTransform(&instance->previousTransform,
                                       &instance->transform, t,
                                       &out[outputCount].transform);
        outputCount++;
    }

    /* ModernPresentSource composites against CapturePrevious(). Vehicles
     * must therefore come from the previous world's visibility/model set too.
     * Using the current list made fast GP-intro camera cuts display a different
     * rival (or no rival) while the rest of the frame was still one tick back. */
    for (uint32_t previousIndex = 0;
         previousIndex < previous->instanceCount && outputCount < capacity;
         previousIndex++) {
        const RageRenderMeshInstance *base =
            &previous->instances[previousIndex];
        const RageRenderMeshInstance *target = NULL;
        uint32_t targetIndex = 0;
        float bestDistance = 0.0f;

        if (!RenderInstanceNeedsSynchronizedMatch(base)) continue;
        for (currentIndex = 0; currentIndex < current->instanceCount;
             currentIndex++) {
            const RageRenderMeshInstance *candidate =
                &current->instances[currentIndex];
            float distance;
            if (matched[currentIndex] ||
                !RenderVehicleIdentityMatches(base, candidate))
                continue;
            distance = RenderTransformDistanceSquared(
                &base->transform, &candidate->transform);
            if (target == NULL || distance < bestDistance) {
                target = candidate;
                targetIndex = currentIndex;
                bestDistance = distance;
            }
        }
        out[outputCount] = *base;
        if (target != NULL) {
            matched[targetIndex] = 1;
            RenderInterpolateTransform(&base->transform,
                                           &target->transform, t,
                                           &out[outputCount].transform);
        }
        outputCount++;
    }
    return outputCount;
}
