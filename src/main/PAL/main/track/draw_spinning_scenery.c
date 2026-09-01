#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/track.h"
#include "rage/render_world_game.h"

void DrawSpinningScenery(s32 timer, s32 animate) {
    Matrix yawMatrix;
    Matrix objectMatrix;
    Matrix renderWorldMtx;
    s32 end;
    s32 start;
    s32 loopIndex;
    s32 limit;
    s32 active;

    active = (g_CourseIndex & 3) != 0;
    if (active) {
        start = 1;
        end = 4;
    } else {
        start = 0;
        end = 1;
    }

    for (loopIndex = start; loopIndex < end; loopIndex++) {
        if (animate != 0) {
            g_SpinningSceneryAngle[loopIndex] +=
                g_SpinningSceneryRate[active];
        }
        g_SpinningSceneryAngle[loopIndex] &= 0xFFF;

        BuildRotMatrixY(&yawMatrix, g_SpinningSceneryYaw[loopIndex].yaw);
        BuildRotMatrixZ(&renderWorldMtx,
                        g_SpinningSceneryAngle[loopIndex]);
        MulMatrix2(&yawMatrix, &renderWorldMtx);
        MulMatrix2((&g_RageScratchpadState.matrix), &yawMatrix);
        BuildRotMatrixZ(&objectMatrix, g_SpinningSceneryAngle[loopIndex]);
        MulMatrix2(&yawMatrix, &objectMatrix);
        SetGteObjectMatrix(
            SCRATCH_OBJECT_MATRIX_WORK,
            AsPosition(&g_SpinningSceneryPos[loopIndex]), &objectMatrix);

        g_RageScratchpadState.envMode4 = 0;
        limit = g_CourseModelCount >= 0x3F ? 0x3E : 1;
        GameRenderWorldSubmitDynamicCourseObject(
            0x100 + loopIndex, limit, g_SpinningSceneryPos[loopIndex].x,
            g_SpinningSceneryPos[loopIndex].y,
            g_SpinningSceneryPos[loopIndex].z, renderWorldMtx.m, 1, 0);
        SubmitCourseModel2((&g_RageScratchpadState), limit);
    }

    if ((timer & 0x1FF) == 0 && animate != 0) {
        g_SpinningSceneryRate[0] = Random15() & 0x1F;
        g_SpinningSceneryRate[1] = Random15() & 0x3F;
    }
}
