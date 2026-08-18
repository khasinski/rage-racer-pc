#include "modern_scene_interpolation.h"

#include <string.h>

#include "../native_geometry_interpolation.h"

static void InterpolateTransform(const RageRenderTransformState *a,
                                 const RageRenderTransformState *b, float t,
                                 RageRenderTransformState *out) {
    int row, column, axis;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            out->rot.m[row][column] =
                (int16_t)(a->rot.m[row][column] +
                          (b->rot.m[row][column] - a->rot.m[row][column]) * t);
            out->light.m[row][column] = b->light.m[row][column];
            out->color.m[row][column] = b->color.m[row][column];
        }
    }
    RageOrthonormalizeMatrix3x3(out->rot.m, b->rot.m);
    for (axis = 0; axis < 3; axis++) {
        out->rot.t[axis] = (int32_t)(a->rot.t[axis] +
            (b->rot.t[axis] - a->rot.t[axis]) * (double)t);
        out->light.t[axis] = b->light.t[axis];
        out->color.t[axis] = b->color.t[axis];
    }
    out->ofx = b->ofx;
    out->ofy = b->ofy;
    out->h = b->h;
    out->dqa = b->dqa;
    out->dqb = b->dqb;
}

void ModernSceneInterpolationPrepare(ModernSceneInterpolation *state,
                                     const RageRenderScene *base,
                                     const RageRenderScene *target, float t) {
    uint8_t matched[RAGE_CAPTURE_MAX_DRAWS] = {0};
    int timerDelta = target->sceneTimer - base->sceneTimer;
    long long cameraDelta = 0;
    int i;
    state->active = 0;
    if (t <= 0.001f || target->sceneId != base->sceneId ||
        timerDelta < 0 || timerDelta > 2) return;
    for (i = 0; i < 3; i++) {
        long long delta = (long long)target->viewPosition[i] -
                          base->viewPosition[i];
        cameraDelta += delta < 0 ? -delta : delta;
    }
    if (cameraDelta > 16384) return;
    for (i = 0; i < base->drawCount; i++) {
        const RageRenderModelDraw *draw = &base->draws[i];
        const RageRenderModelDraw *match = NULL;
        long long bestDistance = 0;
        int j;
        for (j = 0; j < target->drawCount; j++) {
            const RageRenderModelDraw *other = &target->draws[j];
            long long distance = 0;
            int axis;
            if (matched[j] || other->kind != draw->kind ||
                other->modelIndex != draw->modelIndex ||
                other->mirror != draw->mirror || other->table != draw->table)
                continue;
            for (axis = 0; axis < 3; axis++) {
                long long delta = (long long)other->gte.rot.t[axis] -
                                  draw->gte.rot.t[axis];
                distance += delta * delta;
            }
            if (match == NULL || distance < bestDistance) {
                match = other;
                bestDistance = distance;
            }
        }
        if (match != NULL) {
            long dot = 0;
            int axis;
            for (axis = 0; axis < 3; axis++) {
                dot += (long)draw->gte.rot.m[0][axis] * match->gte.rot.m[0][axis];
                dot += (long)draw->gte.rot.m[2][axis] * match->gte.rot.m[2][axis];
            }
            if (dot < 0) match = NULL;
        }
        if (match != NULL) {
            matched[match - target->draws] = 1;
            InterpolateTransform(&draw->gte, &match->gte, t, &state->draws[i]);
        } else {
            state->draws[i] = draw->gte;
        }
    }
    for (i = 0; i < base->terrainCount; i++) {
        const RageRenderTerrainBatch *batch = &base->terrain[i];
        const RageRenderTerrainBatch *match =
            i < target->terrainCount && target->terrain[i].mirror == batch->mirror
                ? &target->terrain[i] : NULL;
        int cell;
        state->terrain[i] = *batch;
        if (match == NULL) continue;
        InterpolateTransform(&batch->gte, &match->gte, t, &state->terrain[i].gte);
        for (cell = 0; cell < batch->cellCount; cell++) {
            int other;
            for (other = 0; other < match->cellCount; other++) {
                if (match->cells[other][3] == batch->cells[cell][3]) {
                    int axis;
                    for (axis = 0; axis < 3; axis++)
                        state->terrain[i].cells[cell][axis] = (int32_t)(
                            batch->cells[cell][axis] +
                            (match->cells[other][axis] - batch->cells[cell][axis]) *
                                (double)t);
                    break;
                }
            }
        }
    }
    state->active = 1;
}

const RageRenderTransformState *ModernSceneInterpolationDraw(
    const ModernSceneInterpolation *state, const RageRenderScene *scene,
    int index) {
    return state->active ? &state->draws[index] : &scene->draws[index].gte;
}

const RageRenderTerrainBatch *ModernSceneInterpolationTerrain(
    const ModernSceneInterpolation *state, const RageRenderScene *scene,
    int index) {
    return state->active ? &state->terrain[index] : &scene->terrain[index];
}
