#include "game/render.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

static void DisableRouteScenery(void) {
    g_RouteSceneryActive = 0;
    g_RouteSceneryFrame = 0;
    g_RouteSceneryKeyIndex = 0;
    g_RouteSceneryKeyframe = NULL;
}

void SeedRouteScenery(void) {
    s32 series;
    s16 firstKeyframe;
    const SceneryMotionKeyframe *keyframe;

    if (g_RouteSceneryData == NULL) {
        DisableRouteScenery();
        return;
    }
    series = g_RaceSeries != 0;
    firstKeyframe = g_RouteSceneryData->firstKeyframe[series][0];
    keyframe = &g_RouteSceneryData->keyframes[firstKeyframe];

    g_RouteSceneryKeyIndex = 0;
    g_RouteSceneryKeyframe = keyframe;
    g_RouteSceneryRotX = keyframe->rotationX;
    g_RouteSceneryRotY = keyframe->rotationY;
    g_RouteSceneryRotZ = keyframe->rotationZ;
    g_RouteSceneryPosition = g_RouteSceneryData->start[series].position;
    g_RouteSceneryActive = 1;
    g_RouteSceneryFrame = 0;
}

void UpdateRouteScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    SVec vin;
    LVec vout;
    const SceneryMotionKeyframe *keyframe;
    s32 elapsed;
    int64_t nextFrame;

    if (g_RouteSceneryActive <= 0) {
        return;
    }
    if (g_RouteSceneryData == NULL || g_RouteSceneryKeyframe == NULL) {
        DisableRouteScenery();
        return;
    }
    keyframe = &g_RouteSceneryKeyframe[g_RouteSceneryKeyIndex];
    nextFrame = g_RouteSceneryFrame < 0
                    ? 1
                    : (int64_t)g_RouteSceneryFrame + 1;
    if (nextFrame >= keyframe->duration) {
        g_RouteSceneryKeyIndex++;
        g_RouteSceneryFrame = 0;
        keyframe++;
    } else {
        g_RouteSceneryFrame = (s32)nextFrame;
    }

    if (keyframe->duration == SCENERY_MOTION_END) {
        SeedRouteScenery();
        keyframe = g_RouteSceneryKeyframe;
    }

    elapsed = g_RouteSceneryFrame;
    g_RouteSceneryRotX = InterpolateSceneryMotionValue(
        keyframe->rotationX, keyframe[1].rotationX, elapsed,
        keyframe->duration);
    g_RouteSceneryRotY = InterpolateSceneryMotionValue(
        keyframe->rotationY, keyframe[1].rotationY, elapsed,
        keyframe->duration);
    g_RouteSceneryRotZ = InterpolateSceneryMotionValue(
        keyframe->rotationZ, keyframe[1].rotationZ, elapsed,
        keyframe->duration);

    vin.vx = 0;
    vin.vy = 0;
    vin.vz = WrapSigned16(-(int64_t)keyframe->speed * 4);
    BuildRotMatrixY(
        &mtx0,
        WrapSigned32((int64_t)0x800 - g_RouteSceneryRotY));

    BuildRotMatrixX(&mtx1, g_RouteSceneryRotX);
    MulMatrix2(&mtx0, &mtx1);
    BuildRotMatrixZ(&mtx0, g_RouteSceneryRotZ);
    MulMatrix(&mtx1, &mtx0);
    ApplyMatrix(&mtx1, &vin, &vout);

    g_RouteSceneryX = WrapSigned32(
        (int64_t)g_RouteSceneryX + vout.x / 4);
    g_RouteSceneryY = WrapSigned32(
        (int64_t)g_RouteSceneryY + vout.y / 4);
    g_RouteSceneryZ = WrapSigned32(
        (int64_t)g_RouteSceneryZ + vout.z / 4);
}
