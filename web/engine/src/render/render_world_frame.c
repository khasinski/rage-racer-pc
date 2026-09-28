#include "render_world_frame.h"

#include <math.h>
#include <string.h>

static float Clamp01(float value) {
    if (!isfinite(value)) return 0.0f;
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void InterpolateVec3(const Vec3 *previous,
                            const Vec3 *current, float t,
                            Vec3 *out) {
    out->x = previous->x + (current->x - previous->x) * t;
    out->y = previous->y + (current->y - previous->y) * t;
    out->z = previous->z + (current->z - previous->z) * t;
}

static float InterpolateWrapped(float previous, float current,
                                float period, float t) {
    if (!isfinite(previous) || !isfinite(current) ||
        !isfinite(period) || period <= 0.0f)
        return isfinite(previous) ? previous : 0.0f;
    float delta = current - previous;
    delta = isfinite(delta) ? fmodf(delta, period)
                           : (float)fmod((double)current - previous, period);
    if (delta > period * 0.5f) delta -= period;
    if (delta < period * -0.5f) delta += period;
    return previous + delta * t;
}

float RenderLerpAngleDegrees(float from, float to, float t) {
    float delta;

    if (!isfinite(from) || !isfinite(to)) return 0.0f;
    t = Clamp01(t);
    delta = to - from;
    delta = isfinite(delta) ? fmodf(delta, 360.0f)
                           : (float)fmod((double)to - (double)from, 360.0);
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return from + delta * t;
}

void RenderInterpolateTransform(const RenderTransform *previous,
                                    const RenderTransform *current,
                                    float t,
                                    RenderTransform *out) {
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

void RenderInterpolateCamera(const RenderCamera *previous,
                                 const RenderCamera *current, float t,
                                 RenderCamera *out) {
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
    out->skyLayout = current->skyLayout;
    out->hasSkyLayout = current->hasSkyLayout;
    if (previous->skyCloudRow != current->skyCloudRow ||
        previous->skyAssetKey != current->skyAssetKey ||
        previous->hasSkyLayout != current->hasSkyLayout ||
        memcmp(&previous->skyLayout, &current->skyLayout,
               sizeof(current->skyLayout)) != 0) {
        out->skyGridOrigin = current->skyGridOrigin;
        out->skyGridColumn = current->skyGridColumn;
        out->skyGridRow = current->skyGridRow;
    } else {
        InterpolateVec3(&previous->skyGridColumn, &current->skyGridColumn, t,
                        &out->skyGridColumn);
        InterpolateVec3(&previous->skyGridRow, &current->skyGridRow, t,
                        &out->skyGridRow);
        {
            float previousTexture = previous->skyGridColumn.z;
            float currentTexture = InterpolateWrapped(
                previousTexture, current->skyGridColumn.z, 32.0f, 1.0f);
            float texture = previousTexture +
                (currentTexture - previousTexture) * t;
            float previousAnchorX = previous->skyGridOrigin.x -
                previousTexture * previous->skyGridColumn.x;
            float previousAnchorY = previous->skyGridOrigin.y -
                previousTexture * previous->skyGridColumn.y;
            float currentAnchorX = current->skyGridOrigin.x -
                currentTexture * current->skyGridColumn.x;
            float currentAnchorY = current->skyGridOrigin.y -
                currentTexture * current->skyGridColumn.y;
            float previousLowerAnchorX = previous->skyGridOrigin.z -
                previousTexture * previous->skyGridColumn.x;
            float previousLowerAnchorY = previous->skyGridRow.z -
                previousTexture * previous->skyGridColumn.y;
            float currentLowerAnchorX = current->skyGridOrigin.z -
                currentTexture * current->skyGridColumn.x;
            float currentLowerAnchorY = current->skyGridRow.z -
                currentTexture * current->skyGridColumn.y;

            /* Origin and texture column jump together at each tile boundary.
             * Interpolate their continuous anchor so the presentation-rate
             * geometry does not jump backwards between logic frames. */
            out->skyGridColumn.z = texture;
            out->skyGridOrigin.x =
                previousAnchorX + (currentAnchorX - previousAnchorX) * t +
                texture * out->skyGridColumn.x;
            out->skyGridOrigin.y =
                previousAnchorY + (currentAnchorY - previousAnchorY) * t +
                texture * out->skyGridColumn.y;
            out->skyGridOrigin.z = previousLowerAnchorX +
                (currentLowerAnchorX - previousLowerAnchorX) * t +
                texture * out->skyGridColumn.x;
            out->skyGridRow.z = previousLowerAnchorY +
                (currentLowerAnchorY - previousLowerAnchorY) * t +
                texture * out->skyGridColumn.y;
        }
    }
    out->fogNear = previous->fogNear +
        (current->fogNear - previous->fogNear) * t;
    out->fogFar = previous->fogFar +
        (current->fogFar - previous->fogFar) * t;
}
