#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

enum {
    START_GRID_ENTITY_ID = 2,
    START_GRID_FIRST_VISIBLE_FRAME = 81,
    START_GRID_MOTION_FRAME = 90,
    START_GRID_FRAMES_PER_MODEL = 3,
    START_GRID_MODEL_COUNT = 15,
    START_GRID_FIRST_MODEL = 0x28,
    START_GRID_HEIGHT_OFFSET = 8,
    SERIES_COURSE_Z_OFFSET = 0x5000,
};

void DrawStartGridScenery(s32 timer) {
    Matrix objectMatrix;
    Matrix worldMatrix;
    Vec4 position;
    s32 animationFrame = 0;
    s32 movementStep = 0;
    s32 modelId;
    s32 series;

    if (g_RacePhase >= RACE_PHASE_ACTIVE ||
        timer < START_GRID_FIRST_VISIBLE_FRAME) {
        return;
    }

    series = g_RaceSeries != 0;
    BuildRotMatrixY(&objectMatrix, g_StartGridSceneryAngle[series]);
    worldMatrix = objectMatrix;
    MulMatrix2(&g_RenderState.geometry.matrix, &objectMatrix);

    position = g_StartGridSceneryPos[series];
    /* Retail ordering let the feet win against the coplanar road. With a
     * depth buffer their lower edge intersects the asphalt, so lift the actor
     * by two imported vertex units. PS1 world Y grows downwards. */
    position.y -= START_GRID_HEIGHT_OFFSET;
    if (timer > START_GRID_MOTION_FRAME) {
        animationFrame =
            (timer - START_GRID_MOTION_FRAME) / START_GRID_FRAMES_PER_MODEL;
        movementStep = animationFrame / START_GRID_MODEL_COUNT;
        position.x = WrapSigned32(
            (int64_t)position.x +
            (int64_t)g_StartGridSceneryStep[series].x * movementStep);
        position.z = WrapSigned32(
            (int64_t)position.z +
            (int64_t)g_StartGridSceneryStep[series].y * movementStep);
        animationFrame -= movementStep * START_GRID_MODEL_COUNT;
    }
    if (SeriesCourseIndex() == 3) {
        position.z = WrapSigned32(
            (int64_t)position.z + SERIES_COURSE_Z_OFFSET);
    }

    modelId = ModelOrFallback(animationFrame + START_GRID_FIRST_MODEL,
                              g_CourseModelCount);
    SetGteObjectMatrix(AsPosition(&position),
                       &objectMatrix);
    g_RenderState.geometry.envMode4 = 0;
    /* Preserve the actor's authored face selection and its OT priority over
     * the road beneath the moving feet. */
    GameRenderWorldSubmitStartGridScenery(
        START_GRID_ENTITY_ID, modelId, position.x, position.y, position.z,
        worldMatrix.m, 0, 0);
    SubmitCourseModel(&g_RenderState, modelId);
}
