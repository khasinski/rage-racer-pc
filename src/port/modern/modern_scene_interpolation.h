#ifndef RAGE_MODERN_SCENE_INTERPOLATION_H
#define RAGE_MODERN_SCENE_INTERPOLATION_H

#include "../render/render_scene.h"

typedef struct ModernSceneInterpolation {
    int active;
    RageRenderTransformState draws[RAGE_CAPTURE_MAX_DRAWS];
    RageRenderTerrainBatch terrain[RAGE_CAPTURE_MAX_TERRAIN];
} ModernSceneInterpolation;

void ModernSceneInterpolationPrepare(ModernSceneInterpolation *state,
                                     const RageRenderScene *base,
                                     const RageRenderScene *target, float t);
const RageRenderTransformState *ModernSceneInterpolationDraw(
    const ModernSceneInterpolation *state, const RageRenderScene *scene,
    int index);
const RageRenderTerrainBatch *ModernSceneInterpolationTerrain(
    const ModernSceneInterpolation *state, const RageRenderScene *scene,
    int index);

#endif
