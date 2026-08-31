#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/player_car_internal.h"
#include "game/track_internal.h"
#include "game/state.h"
#include "rage/render_world_game.h"

void DrawStartGridScenery(s32 flags) {
    Matrix mtx;
    Matrix renderWorldMtx;
    Vec4 state;
    s32 s1;
    s32 s0;
    s32 value;
    s32 drawArg;
    s32 rem;
    s32 lim;

    if (g_RacePhase < 2 && flags >= 0x51) {
        BuildRotMatrixY(&mtx, g_StartGridSceneryAngle[ReadStableRaceSeries()]);
        renderWorldMtx = mtx;
        MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx);
        if (flags - 90 > 0) {
            state = g_StartGridSceneryPos[ReadStableRaceSeries()];
            s1 = (flags - 90) / 3;
            state.x +=
                g_StartGridSceneryStep[ReadStableRaceSeries()].x * (s0 = s1 / 15);
            state.z += g_StartGridSceneryStep[ReadStableRaceSeries()].y * s0;
            if (SeriesCourseIndex() == 3) {
                state.z += 0x5000;
            }
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
            rem = s1 - s0 * 15;
            lim = g_CourseModelCount;
            
            value = rem + 0x28;
            SCRATCH_ENV_MODE4 = 0;
            drawArg = (value < lim) ? value : 1;
        } else {
            state = g_StartGridSceneryPos[ReadStableRaceSeries()];
            if (SeriesCourseIndex() == 3) {
                state.z += 0x5000;
            }
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
            lim = g_CourseModelCount;
            
            value = 0x28;
            SCRATCH_ENV_MODE4 = 0;
            drawArg = (value < lim) ? value : 1;
        }
        GameRenderWorldSubmitDynamicCourseObject(
            2, drawArg, state.x, state.y, state.z, renderWorldMtx.m, 0, 0);
        SubmitCourseModel(SCRATCHPAD, drawArg);
    }
}


void DrawAnimatedScenery(s32 timer, s32 instance) {
    Matrix mtx;
    Matrix mtx2;
    Matrix renderWorldMtx;
    Vec4 state;
    s32 visible;
    s32 num;
    s32 drawArg;

    state = g_AnimSceneryPos[instance];

    if ((SeriesCourseIndex()) == 3) {
        state.z += 0x5000;
    }
    if (g_GrandPrixClass == 5) {
        return;
    }

    visible = TrackCellVisible(state.x, state.z);
    if (visible == 0) {
        return;
    }

    g_AnimSceneryFrame = (timer / 4) % 16;
    if (g_AnimSceneryFrame == 0 && (timer % 8) == 0 && g_RacePaused == 0) {
        g_AnimSceneryTint = 0;
        g_AnimSceneryRacePosition = g_RacePosition;
        g_AnimSceneryVariant = (Random15() & 7) / 3;
        if (g_AnimSceneryRacePosition >= 4) {
            g_AnimSceneryRacePosition = 0;
        }
    }

    BuildRotMatrixY(&mtx, state.w);
    BuildRotMatrixX(&mtx2, g_AnimSceneryPitch[instance]);
    MulMatrix(&mtx, &mtx2);
    renderWorldMtx = mtx;
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx);

    if (g_GrandPrixMode == 0) {
        return;
    }

    g_AnimSceneryTint = ((timer >> 3) & 3) << 16;

    if (g_AnimSceneryRacePosition != 0) {
        if (g_AnimSceneryFrame < 13) {
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
            num = g_AnimSceneryFrame + 10;
            SCRATCH_ENV_MODE4 = 0;
            drawArg = (num < g_CourseModelCount) ? num : 1;
            GameRenderWorldSubmitDynamicCourseOverlay(
                0x20 + instance * 2, drawArg, state.x, state.y, state.z,
                renderWorldMtx.m, 0, 0);
            SubmitCourseModel(SCRATCHPAD, drawArg);
        } else {
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
            num = g_AnimSceneryRacePosition;
            SCRATCH_ENV_MODE4 = 0;
            drawArg = (num < g_CourseModelCount) ? num : 1;
            GameRenderWorldSubmitDynamicCourseOverlay(
                0x20 + instance * 2, drawArg, state.x, state.y, state.z,
                renderWorldMtx.m, 0, 0);
            SubmitCourseModel(SCRATCHPAD, drawArg);
        }

        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        SCRATCH_ENV_MODE4 = g_AnimSceneryTint;
        num = g_AnimSceneryVariant + 4;
        drawArg = (num < g_CourseModelCount) ? num : 1;
        GameRenderWorldSubmitDynamicCourseOverlay(
            0x21 + instance * 2, drawArg, state.x, state.y, state.z,
            renderWorldMtx.m, 0, 0);
        SubmitCourseModel(SCRATCHPAD, drawArg);
    } else {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        num = g_AnimSceneryFrame + 0x18;
        SCRATCH_ENV_MODE4 = 0;
        drawArg = (num < g_CourseModelCount) ? num : 1;
        GameRenderWorldSubmitDynamicCourseOverlay(
            0x20 + instance * 2, drawArg, state.x, state.y, state.z,
            renderWorldMtx.m, 0, 0);
        SubmitCourseModel(SCRATCHPAD, drawArg);

        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        SCRATCH_ENV_MODE4 = g_AnimSceneryTint;
        num = g_AnimSceneryVariant + 7;
        drawArg = (num < g_CourseModelCount) ? num : 1;
        GameRenderWorldSubmitDynamicCourseOverlay(
            0x21 + instance * 2, drawArg, state.x, state.y, state.z,
            renderWorldMtx.m, 0, 0);
        SubmitCourseModel(SCRATCHPAD, drawArg);
    }
}

void DrawAnimatedScenery2(s32 timer, s32 instance, s32 isReplay, s32 animate) {
    Matrix mtx;
    Matrix mtx2;
    Matrix renderWorldMtx;
    Vec4 state;
    s32 visible;
    s32 num;
    s32 drawArg;
    s32 sv;

    if (g_GrandPrixMode == 0) {
        return;
    }
    if (g_GrandPrixClass == 5) {
        return;
    }

    state = g_AnimSceneryPos[instance];
    if ((SeriesCourseIndex()) == 3) {
        state.z += 0x5000;
    }

    visible = TrackCellVisible(state.x, state.z);
    if (visible == 0) {
        return;
    }

    g_AnimScenery2Frame = (timer / 4) % 16;
    if (g_AnimScenery2Frame == 0 && (timer % 8) == 0 && animate == 1) {
        g_AnimScenery2Tint = 0;
        g_AnimScenery2Variant = (Random15() & 7) / 3;
    }

    BuildRotMatrixY(&mtx, state.w);
    BuildRotMatrixX(&mtx2, g_AnimSceneryPitch[instance]);
    MulMatrix(&mtx, &mtx2);
    renderWorldMtx = mtx;
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx);

    g_AnimScenery2Tint = ((timer >> 3) & 3) << 16;

    if (isReplay != 0) {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        num = g_AnimScenery2Frame + 0xA;
        SCRATCH_ENV_MODE4 = 0;
        drawArg = (num < g_CourseModelCount) ? num : 1;
        GameRenderWorldSubmitDynamicCourseOverlay(
            0x30 + instance * 2, drawArg, state.x, state.y, state.z,
            renderWorldMtx.m, 0, 0);
        SubmitCourseModel(SCRATCHPAD, drawArg);

        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        sv = g_AnimScenery2Tint;
        drawArg = 1;
        SCRATCH_ENV_MODE4 = sv;
        sv = g_CourseModelCount;
        num = g_AnimScenery2Variant + 4;
    } else {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        num = g_AnimScenery2Frame + 0x18;
        SCRATCH_ENV_MODE4 = 0;
        drawArg = (num < g_CourseModelCount) ? num : 1;
        GameRenderWorldSubmitDynamicCourseOverlay(
            0x30 + instance * 2, drawArg, state.x, state.y, state.z,
            renderWorldMtx.m, 0, 0);
        SubmitCourseModel(SCRATCHPAD, drawArg);

        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPosition(&state), &mtx);
        sv = g_AnimScenery2Tint;
        drawArg = 1;
        SCRATCH_ENV_MODE4 = sv;
        sv = g_CourseModelCount;
        num = g_AnimScenery2Variant + 7;
    }

    if (num < sv) {
        drawArg = num;
    }
    GameRenderWorldSubmitDynamicCourseOverlay(
        0x31 + instance * 2, drawArg, state.x, state.y, state.z,
        renderWorldMtx.m, 0, 0);
    SubmitCourseModel(SCRATCHPAD, drawArg);
}
