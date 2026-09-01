#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"


void DrawStaticScenery(s32 shifted) {
    SceneryPlacement *placement = &g_StaticSceneryState.standard;
    Matrix mtx;
    Matrix renderWorldMtx;
    Vec4 state = {
        placement->position.x,
        placement->position.y,
        placement->position.z,
        0,
    };
    s32 visible;
    s16 drawArg;
    s32 frameValue;

    if (shifted != 0) {
        state.z += 0x5000;
    }

    visible = TrackCellVisible(state.x, state.z);

    if (visible != 0) {
        BuildRotMatrixY(&mtx, placement->yaw);
        renderWorldMtx = mtx;
        MulMatrix2((&g_RenderState.matrix), &mtx);

        if (g_IsEnvironmentMode4 != 0) {
            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPosition(&state), &mtx);
            frameValue = g_CourseModelCount;
            g_RenderState.envMode4 = 0;
            drawArg = 1;
            if (frameValue >= 0x3B) {
                drawArg = 0x3A;
            }
            GameRenderWorldSubmitDynamicCourseObject(
                0, drawArg, state.x, state.y, state.z,
                renderWorldMtx.m, 0, 0);
            SubmitCourseModel((&g_RenderState), drawArg);
        } else {
            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPosition(&state), &mtx);
            frameValue = g_CourseModelCount;
            g_RenderState.envMode4 = 0;
            drawArg = 1;
            if (frameValue >= 0x3A) {
                drawArg = 0x39;
            }
            GameRenderWorldSubmitDynamicCourseObject(
                0, drawArg, state.x, state.y, state.z,
                renderWorldMtx.m, 1, 0);
            SubmitCourseModel2((&g_RenderState), drawArg);
        }
    }
}

void DrawHighClassScenery(void) {
    SceneryPlacement *placement = &g_StaticSceneryState.highClass;
    Matrix mtx;
    Matrix renderWorldMtx;
    s32 drawArg;

    BuildRotMatrixY(&mtx, placement->yaw);
    renderWorldMtx = mtx;
    MulMatrix2((&g_RenderState.matrix), &mtx);

    if (g_IsEnvironmentMode4 != 0) {
        SetGteObjectMatrix(&g_ObjectMatrixWork, &placement->position, &mtx);
        g_RenderState.envMode4 = 0x10000;
        drawArg = 1;
        if (g_CourseModelCount >= 0x40) {
            drawArg = 0x3F;
        }
        GameRenderWorldSubmitDynamicCourseObject(
            1, drawArg, placement->position.x, placement->position.y,
            placement->position.z,
            renderWorldMtx.m, 0, 0);
        SubmitCourseModel((&g_RenderState), drawArg);
    } else {
        SetGteObjectMatrix(&g_ObjectMatrixWork, &placement->position, &mtx);
        g_RenderState.envMode4 = 0;
        drawArg = 1;
        if (g_CourseModelCount >= 0x40) {
            drawArg = 0x3F;
        }
        GameRenderWorldSubmitDynamicCourseObject(
            1, drawArg, placement->position.x, placement->position.y,
            placement->position.z,
            renderWorldMtx.m, 1, 0);
        SubmitCourseModel2((&g_RenderState), drawArg);
    }
}

void DrawCourseScenery(s32 course, s32 timer, s32 animate) {
    s32 flag = animate;

    DrawAnimatedScenery(timer, 0);

    if (g_GrandPrixClass == 5) {
        flag = 0;
    }

    switch (course) {
    case 0:
        DrawSpinningScenery(timer, flag);
        if (g_GrandPrixClass >= 4) {
            DrawHighClassScenery();
        }
        DrawStaticScenery(0);
        break;
    case 1:
        if (g_GrandPrixClass >= 2) {
            DrawSpinningScenery(timer, flag);
        }
        if (flag != 0) {
            UpdateShuttleScenery(0);
        }
        DrawShuttleScenery(0);
        DrawStaticScenery(0);
        break;
    case 2:
        if (flag != 0) {
            UpdateShuttleScenery(0);
            UpdateShuttleScenery(1);
        }
        DrawShuttleScenery(0);
        DrawShuttleScenery(1);
        DrawStaticScenery(0);
        break;
    case 3:
        DrawAnimatedScenery(timer, 1);
        DrawStaticScenery(1);
        break;
    default:
        break;
    }
}

void DrawCourseScenery2(s32 timer, s32 animate) {
    s32 flag = animate;
    s32 mode;

    if (g_GrandPrixClass == 5) {
        flag = 0;
    }

    DrawAnimatedScenery2(timer, 0, g_SceneId == 0x11, flag);

    mode = SeriesCourseIndex();
    switch (mode) {
    case 0:
        DrawSpinningScenery(timer, flag);
        if (g_GrandPrixClass >= 4) {
            DrawHighClassScenery();
        }
        DrawStaticScenery(0);
        break;
    case 1:
        if (g_GrandPrixClass >= 2) {
            DrawSpinningScenery(timer, flag);
        }
        if (flag != 0) {
            UpdateShuttleScenery(0);
        }
        DrawShuttleScenery(0);
        DrawStaticScenery(0);
        break;
    case 2:
        if (flag != 0) {
            UpdateShuttleScenery(0);
            UpdateShuttleScenery(1);
        }
        DrawShuttleScenery(0);
        DrawShuttleScenery(1);
        DrawStaticScenery(0);
        break;
    case 3:
        DrawAnimatedScenery2(timer, 1, g_SceneId == 0x11, flag);
        DrawStaticScenery(1);
        break;
    default:
        break;
    }
}
