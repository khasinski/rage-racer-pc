#include "modern/modern_scene_interpolation.h"

static RageRenderScene baseScene;
static RageRenderScene targetScene;
static ModernSceneInterpolation interpolation;

static void Identity(RageRenderTransformState *state, int x) {
    state->rot.m[0][0] = 4096;
    state->rot.m[1][1] = 4096;
    state->rot.m[2][2] = 4096;
    state->rot.t[0] = x;
}

int main(void) {
    const RageRenderTransformState *draw;
    baseScene.sceneId = targetScene.sceneId = 12;
    baseScene.sceneTimer = 100;
    targetScene.sceneTimer = 101;
    baseScene.drawCount = targetScene.drawCount = 2;
    baseScene.draws[0].modelIndex = baseScene.draws[1].modelIndex = 7;
    targetScene.draws[0].modelIndex = targetScene.draws[1].modelIndex = 7;
    Identity(&baseScene.draws[0].gte, 0);
    Identity(&baseScene.draws[1].gte, 100);
    /* Reversed target order proves one-to-one nearest matching. */
    Identity(&targetScene.draws[0].gte, 110);
    Identity(&targetScene.draws[1].gte, 10);
    ModernSceneInterpolationPrepare(&interpolation, &baseScene, &targetScene,
                                    0.5f);
    if (!interpolation.active) return 1;
    draw = ModernSceneInterpolationDraw(&interpolation, &baseScene, 0);
    if (draw->rot.t[0] != 5) return 2;
    draw = ModernSceneInterpolationDraw(&interpolation, &baseScene, 1);
    if (draw->rot.t[0] != 105) return 3;

    targetScene.sceneId = 17;
    ModernSceneInterpolationPrepare(&interpolation, &baseScene, &targetScene,
                                    0.5f);
    if (interpolation.active) return 4;
    targetScene.sceneId = 12;
    targetScene.viewPosition[0] = 20000;
    ModernSceneInterpolationPrepare(&interpolation, &baseScene, &targetScene,
                                    0.5f);
    if (interpolation.active) return 5;
    return 0;
}
