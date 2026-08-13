#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/asset_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "psyq/gte.h"

#ifdef __psyz
#include <stdio.h>
#include <stdlib.h>
#endif



/*
 * GameRenderObject -> GPU-primitive submitter. Subtracts the active view's
 * horizon from the object's y, builds a stack of rotation matrices from the
 * object's angle sets, loads each transform into the GTE at 0x1F80011C and
 * dispatches the primitive builder SubmitModel on the scratchpad OT
 * (0x1F800000) at increasing depth buckets. The m_90 negation block and the
 * m_B0[1] block build the mirrored copies (flip X/Z columns). otDepth is the
 * base OT bucket; clipHandle is the optional clip volume from GetTrackZoneBlend.
 */
void DrawPlayerCarModel(GameRenderObject *obj) {
    CarModelAsset *view = g_CarModelAsset;
    Matrix m_10;
    Matrix m_30;
    Matrix m_50;
    Matrix m_70;
    Matrix m_90;
    Matrix m_B0[2];
    Matrix m_F0;
    s16 v_110[4];
    s32 m_118[8];
    s32 v_138[3];
    s32 unused_144[4]; /* reserved stack slot present in the original */
    s32 clipHandle = 0;
    s32 otDepth;
    s32 i;

    obj->y -= view->horizon;
    obj->modelY -= view->horizon;
    BuildRotMatrixY(&m_10, 0x800 - obj->angleY);
    BuildRotMatrixX(&m_30, obj->bodyPitch);
    MulMatrix2(&m_10, &m_30);
    MulMatrix0(&g_SceneLightMatrix, &m_30, &m_90);

    if (g_SceneId != 8) {
        clipHandle = GetTrackZoneBlend(obj->trackProgress);
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, &m_90);
        }
    }
    SetLightMatrix(&m_90);

    m_90.m[0][0] = -m_90.m[0][0];
    m_90.m[0][2] = -m_90.m[0][2];
    m_90.m[1][0] = -m_90.m[1][0];
    m_90.m[1][2] = -m_90.m[1][2];
    m_90.m[2][0] = -m_90.m[2][0];
    m_90.m[2][2] = -m_90.m[2][2];

    m_50 = m_30;
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &m_30);

    BuildRotMatrixY(&m_10, 0x800 - obj->modelYaw);
    BuildRotMatrixX(&m_70, obj->modelPitch);
    MulMatrix2(&m_10, &m_70);
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &m_70);
    BuildRotMatrixZ(&m_10, obj->modelRoll);
    MulMatrix2(&m_70, &m_10);

    v_138[0] = obj->x;
    v_138[2] = obj->z;
    v_138[1] = obj->modelY;
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, v_138, &m_10);
    g_ScratchRenderMode = 0;
    SubmitModel(SCRATCHPAD, 1);

    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, v_138, &m_10);
    g_ScratchRenderMode = 0;
    SubmitModel(SCRATCHPAD, 1);

    BuildRotMatrixZ(&m_70, obj->bodyRoll);
    MulMatrix2(&m_30, &m_70);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, obj, &m_70);
    g_ScratchRenderMode = 0;
    SubmitModel(SCRATCHPAD, g_ModelBankCount < 1);

    otDepth = obj->renderDepth * 2;
    if (obj->wheelRotation & 0x1000) {
        otDepth += 10;
    }
    BuildRotMatrixZ(&m_10, obj->bodyRoll - obj->bodyRollVelocity);
    MulMatrix(&m_50, &m_10);
    MulMatrix(&m_30, &m_10);
    BuildRotMatrixX(&m_F0, obj->wheelRotation);
    MulMatrix2(&m_30, &m_F0);

    BuildRotMatrixY(&m_10, obj->steeringAngle / 12);
    BuildRotMatrixX(&m_B0[0], obj->wheelRotation);
    MulMatrix2(&m_10, &m_B0[0]);
    MulMatrix2(&m_30, &m_B0[0]);

    m_B0[1].m[0][0] = -m_B0[0].m[0][0];
    m_B0[1].m[0][1] = m_B0[0].m[0][1];
    m_B0[1].m[0][2] = -m_B0[0].m[0][2];
    m_B0[1].m[1][0] = -m_B0[0].m[1][0];
    m_B0[1].m[1][1] = m_B0[0].m[1][1];
    m_B0[1].m[1][2] = -m_B0[0].m[1][2];
    m_B0[1].m[2][0] = -m_B0[0].m[2][0];
    m_B0[1].m[2][1] = m_B0[0].m[2][1];
    m_B0[1].m[2][2] = -m_B0[0].m[2][2];
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, obj, &m_F0);
    g_ScratchRenderMode = 0;
    SubmitModel(SCRATCHPAD, (otDepth + 3 < g_ModelBankCount) ? (otDepth + 3) : 1);

    for (i = 0; i < 2; i++) {
        CarModelAsset *v = g_CarModelAsset;
        s32 ax = v->modelOffsetX;
        if (i % 2) {
            ax = -ax;
        }
        v_110[0] = ax;
        v_110[1] = v->modelOffsetY;
        v_110[2] = v->modelOffsetZ;
        ApplyMatrix(&m_50, v_110, m_118);
        m_118[0] += obj->x;
        m_118[1] += obj->y;
        m_118[2] += obj->z;
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, m_118, &m_B0[i]);
        g_ScratchRenderMode = 0;
        SubmitModel(SCRATCHPAD, (otDepth + 2 < g_ModelBankCount) ? (otDepth + 2) : 1);
        SetLightMatrix(&m_90);
    }

    obj->y += g_CarModelAsset->horizon;
    obj->modelY += g_CarModelAsset->horizon;
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}


void DrawCar(GameRenderObject *obj) {
    Matrix m_10;
    Matrix m_30;
    Matrix m_50;
    Matrix m_70;
    Matrix m_90;
    Matrix m_B0[2];
    Matrix m_F0;
    s16 v_110[4];
    s32 m_118[4];
    s32 v_128[4];
    s32 v_138[4];
    s32 v_148[4];
    s32 clipHandle = 0;
    s32 otDepth;
    s32 i;
    s32 model;
    s16 *lod;

    model = g_CarModelByCourse[g_CourseIndex][obj->modelIndex];
    lod = g_CarModelBankTable[model];
    obj->y -= GetCamRow(g_CamRow, model)->horizon;
    obj->modelY -= GetCamRow(g_CamRow, model)->horizon;

    v_128[0] = obj->x - SCRATCH_VIEW_X;
    v_128[1] = 0;
    v_128[2] = obj->z - SCRATCH_VIEW_Z;
    ApplyMatrixLV(SCRATCH_VIEW_MATRIX_GTE, v_128, v_148);
#ifdef __psyz
    if (getenv("RAGE_PORT_CAR_DRAW_TRACE") != NULL &&
        g_RageScratchpadState.mode == 9) {
        const char *timerText = getenv("RAGE_PORT_CAR_DRAW_TRACE_TIMER");
        if (timerText == NULL || g_SceneTimer == (s32)strtol(timerText, NULL, 0)) {
            printf("car-draw timer=%d mirror=1 index=%ld "
                   "matrix=%d,%d,%d,%d,%d,%d,%d,%d,%d "
                   "input=%d,%d,%d output=%d,%d,%d\n", g_SceneTimer,
                   (long)(((GameCarRuntime *)(void *)obj - g_Cars)),
                   SCRATCH_VIEW_MATRIX_GTE->m[0][0], SCRATCH_VIEW_MATRIX_GTE->m[0][1],
                   SCRATCH_VIEW_MATRIX_GTE->m[0][2], SCRATCH_VIEW_MATRIX_GTE->m[1][0],
                   SCRATCH_VIEW_MATRIX_GTE->m[1][1], SCRATCH_VIEW_MATRIX_GTE->m[1][2],
                   SCRATCH_VIEW_MATRIX_GTE->m[2][0], SCRATCH_VIEW_MATRIX_GTE->m[2][1],
                   SCRATCH_VIEW_MATRIX_GTE->m[2][2], v_128[0], v_128[1], v_128[2],
                   v_148[0], v_148[1], v_148[2]);
        }
    }
#endif
    if (v_128[0] < 0) {
        v_128[0] = -v_128[0];
    }
    if (v_128[2] < 0) {
        v_128[2] = -v_128[2];
    }
    otDepth = v_128[0] + v_128[2];
    if (v_148[2] >= 0) {
        if (otDepth < 0xD00) {
            BuildRotMatrixY(&m_10, 0x800 - obj->angleY);
            BuildRotMatrixX(&m_30, obj->bodyPitch);
            MulMatrix2(&m_10, &m_30);
            MulMatrix0(&g_SceneLightMatrix, &m_30, &m_90);
            clipHandle = GetTrackZoneBlend(obj->trackProgress);
            if (clipHandle != 0) {
                ApplyZoneLighting(clipHandle, &m_90);
            }
            SetLightMatrix(&m_90);

            m_90.m[0][0] = -m_90.m[0][0];
            m_90.m[0][2] = -m_90.m[0][2];
            m_90.m[1][0] = -m_90.m[1][0];
            m_90.m[1][2] = -m_90.m[1][2];
            m_90.m[2][0] = -m_90.m[2][0];
            m_90.m[2][2] = -m_90.m[2][2];

            m_50 = m_30;
            MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &m_30);

            BuildRotMatrixY(&m_10, 0x800 - obj->modelYaw);
            BuildRotMatrixX(&m_70, obj->modelPitch);
            MulMatrix2(&m_10, &m_70);
            MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &m_70);
            BuildRotMatrixZ(&m_10, obj->modelRoll);
            MulMatrix2(&m_70, &m_10);

            v_138[0] = obj->x;
            v_138[2] = obj->z;
            v_138[1] = obj->modelY;
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, v_138, &m_10);
            g_ScratchRenderMode = 0;
            SubmitModel(SCRATCHPAD,
                            (lod[0] + 1 < g_ModelBankCount) ? (lod[0] + 1) : 1);

            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, v_138, &m_10);
            g_ScratchRenderMode = 0;
            SubmitModel(SCRATCHPAD,
                            (lod[0] + 1 < g_ModelBankCount) ? (lod[0] + 1) : 1);

            BuildRotMatrixZ(&m_70, obj->bodyRoll);
            MulMatrix2(&m_30, &m_70);
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, obj, &m_70);
            g_ScratchRenderMode = lod[1] << 16;
            SubmitModel(SCRATCHPAD,
                            (lod[0] < g_ModelBankCount) ? lod[0] : 1);

            BuildRotMatrixZ(&m_10, obj->bodyRoll - obj->bodyRollVelocity);
            MulMatrix(&m_50, &m_10);
            MulMatrix(&m_30, &m_10);
            BuildRotMatrixX(&m_F0, obj->wheelRotation);
            MulMatrix2(&m_30, &m_F0);

            BuildRotMatrixY(&m_10, obj->steeringAngle * 2);
            BuildRotMatrixX(&m_B0[0], obj->wheelRotation);
            MulMatrix2(&m_10, &m_B0[0]);
            MulMatrix2(&m_30, &m_B0[0]);

            m_B0[1].m[0][0] = -m_B0[0].m[0][0];
            m_B0[1].m[0][1] = m_B0[0].m[0][1];
            m_B0[1].m[0][2] = -m_B0[0].m[0][2];
            m_B0[1].m[1][0] = -m_B0[0].m[1][0];
            m_B0[1].m[1][1] = m_B0[0].m[1][1];
            m_B0[1].m[1][2] = -m_B0[0].m[1][2];
            m_B0[1].m[2][0] = -m_B0[0].m[2][0];
            m_B0[1].m[2][1] = m_B0[0].m[2][1];
            m_B0[1].m[2][2] = -m_B0[0].m[2][2];
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, obj, &m_F0);
            g_ScratchRenderMode = 0;
            SubmitModel(SCRATCHPAD,
                            (lod[0] + 3 < g_ModelBankCount) ? (lod[0] + 3) : 1);

            for (i = 0; i < 2; i++) {
                s32 ax = GetCamRow(g_CamRow, model)->axis0;
                if (i % 2) {
                    ax = -ax;
                }
                v_110[0] = ax;
                v_110[1] = GetCamRow(g_CamRow, model)->axis1;
                v_110[2] = GetCamRow(g_CamRow, model)->axis2;
                ApplyMatrix(&m_50, v_110, m_118);
                m_118[0] += obj->x;
                m_118[1] += obj->y;
                m_118[2] += obj->z;
                SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, m_118, &m_B0[i]);
                g_ScratchRenderMode = 0;
                SubmitModel(SCRATCHPAD,
                                (lod[0] + 2 < g_ModelBankCount) ? (lod[0] + 2) : 1);
                SetLightMatrix(&m_90);
            }
        } else if (otDepth < 0x2500) {
            BuildRotMatrixY(&m_10, 0x800 - obj->angleY);
            BuildRotMatrixX(&m_50, obj->bodyPitch);
            MulMatrix2(&m_10, &m_50);
            MulMatrix0(&g_SceneLightMatrix, &m_50, &m_90);
            clipHandle = GetTrackZoneBlend(obj->trackProgress);
            if (clipHandle != 0) {
                ApplyZoneLighting(clipHandle, &m_90);
            }
            SetLightMatrix(&m_90);

            BuildRotMatrixZ(&m_10, obj->bodyRoll);
            MulMatrix2(&m_50, &m_10);
            MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &m_10);
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, obj, &m_10);
            g_ScratchRenderMode = lod[1] << 16;
            SubmitModel(SCRATCHPAD,
                            (lod[0] + 4 < g_ModelBankCount) ? (lod[0] + 4) : 1);
        }
    }

    obj->y += GetCamRow(g_CamRow, model)->horizon;
    obj->modelY += GetCamRow(g_CamRow, model)->horizon;
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}
