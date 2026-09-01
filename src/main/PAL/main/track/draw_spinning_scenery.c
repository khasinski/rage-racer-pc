#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/track.h"
#include "rage/render_world_game.h"

void DrawSpinningScenery(s32 timer, s32 animate) {
    Matrix yawMatrix;
    Matrix objectMatrix;
    Matrix worldMatrix;
    s32 firstSpinner;
    s32 spinnerLimit;
    s32 spinner;
    s32 modelId;
    s32 multipleSpinners;

    multipleSpinners = SeriesCourseIndex() != 0;
    if (multipleSpinners) {
        firstSpinner = 1;
        spinnerLimit = 4;
    } else {
        firstSpinner = 0;
        spinnerLimit = 1;
    }

    for (spinner = firstSpinner; spinner < spinnerLimit; spinner++) {
        SpinningSceneryPlacement *placement =
            &g_SpinningSceneryPlacements[spinner];

        if (animate != 0) {
            g_SpinningSceneryAngle[spinner] +=
                g_SpinningSceneryRate[multipleSpinners];
        }
        g_SpinningSceneryAngle[spinner] &= 0xFFF;

        BuildRotMatrixY(&yawMatrix, placement->yaw);
        BuildRotMatrixZ(&worldMatrix, g_SpinningSceneryAngle[spinner]);
        MulMatrix2(&yawMatrix, &worldMatrix);
        MulMatrix2(&g_RenderState.matrix, &yawMatrix);
        BuildRotMatrixZ(&objectMatrix, g_SpinningSceneryAngle[spinner]);
        MulMatrix2(&yawMatrix, &objectMatrix);
        SetGteObjectMatrix(&g_ObjectMatrixWork, &placement->position,
                           &objectMatrix);

        g_RenderState.envMode4 = 0;
        modelId = g_CourseModelCount >= 0x3F ? 0x3E : 1;
        GameRenderWorldSubmitDynamicCourseObject(
            0x100 + spinner, modelId, placement->position.x,
            placement->position.y, placement->position.z,
            worldMatrix.m, 1, 0);
        SubmitCourseModel2(&g_RenderState, modelId);
    }

    if ((timer & 0x1FF) == 0 && animate != 0) {
        g_SpinningSceneryRate[0] = Random15() & 0x1F;
        g_SpinningSceneryRate[1] = Random15() & 0x3F;
    }
}
