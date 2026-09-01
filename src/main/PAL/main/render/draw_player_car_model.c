#include "game/diagnostics.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "rage/render_world_game.h"

#include <stdio.h>
#include <stdlib.h>


/*
 * GameRenderObject -> GPU-primitive submitter. Subtracts the active view's
 * horizon from the object's y, builds a stack of rotation matrices from the
 * object's angle sets, loads each transform into the GTE at 0x1F80011C and
 * dispatches the primitive builder SubmitModel on the render state's OT
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
    LVec modelPosition;
    s32 clipHandle = 0;
    s32 otDepth;
    s32 i;

    GameRenderWorldSubmitPlayerCar(obj, g_RenderState.orderingFlag != 0);

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
    MulMatrix2((&g_RenderState.matrix), &m_30);

    BuildRotMatrixY(&m_10, 0x800 - obj->modelYaw);
    BuildRotMatrixX(&m_70, obj->modelPitch);
    MulMatrix2(&m_10, &m_70);
    MulMatrix2((&g_RenderState.matrix), &m_70);
    BuildRotMatrixZ(&m_10, obj->modelRoll);
    MulMatrix2(&m_70, &m_10);

    modelPosition.x = obj->x;
    modelPosition.z = obj->z;
    modelPosition.y = obj->modelY;
    SetGteObjectMatrix((&g_ObjectMatrixWork), &modelPosition, &m_10);
    g_ScratchRenderMode = 0;
    SubmitModel((&g_RenderState), 1);

    SetGteObjectMatrix((&g_ObjectMatrixWork), &modelPosition, &m_10);
    g_ScratchRenderMode = 0;
    SubmitModel((&g_RenderState), 1);

    BuildRotMatrixZ(&m_70, obj->bodyRoll);
    MulMatrix2(&m_30, &m_70);
    SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_70);
    g_ScratchRenderMode = 0;
    SubmitModel((&g_RenderState), g_ModelBankCount < 1);

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
    SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_F0);
    g_ScratchRenderMode = 0;
    SubmitModel((&g_RenderState), (otDepth + 3 < g_ModelBankCount) ? (otDepth + 3) : 1);

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
        SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(m_118), &m_B0[i]);
        g_ScratchRenderMode = 0;
        SubmitModel((&g_RenderState), (otDepth + 2 < g_ModelBankCount) ? (otDepth + 2) : 1);
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

    model = g_CarModelByCourse[SeriesCourseIndex()][obj->modelIndex];
    lod = g_CarModelBankTable[model];

    v_128[0] = obj->x - g_RenderState.viewX;
    v_128[1] = 0;
    v_128[2] = obj->z - g_RenderState.viewZ;
    ApplyMatrixLV((&g_RenderState.matrix), v_128, v_148);
    if (v_128[0] < 0) {
        v_128[0] = -v_128[0];
    }
    if (v_128[2] < 0) {
        v_128[2] = -v_128[2];
    }
    otDepth = v_128[0] + v_128[2];
    if (DiagnosticsEnabled("render.car_draw_trace")) {
        const char *timerText = DiagnosticsValue("render.car_draw_trace_timer");
        if (timerText == NULL || g_SceneTimer == (s32)strtol(timerText, NULL, 0)) {
            const char *detail = v_148[2] < 0 ? "behind" :
                                 otDepth < 0xD00 ? "close" :
                                 otDepth < 0x2500 ? "far" : "culled";
            Trace("car-draw", "timer=%d mirror=%d index=%ld source=%d "
                   "car=%d lod=%d palette=%d depth=%d view-z=%d detail=%s "
                   "player=%d grade=%d asset=%d",
                   g_SceneTimer, g_RenderState.orderingFlag != 0,
                   (long)(((GameCarRuntime *)(void *)obj - g_Cars)),
                   obj->modelIndex, model, lod[0], lod[1], otDepth,
                   v_148[2], detail, g_PlayerCarIndex,
                   g_CarTable[g_PlayerCarIndex].modelVariant,
                   GetCarAssetIndex(g_PlayerCarIndex,
                       g_CarTable[g_PlayerCarIndex].modelVariant));
        }
    }
    if (v_148[2] >= 0 && otDepth < 0x2500) {
        GameRenderWorldSubmitCar(
            obj, g_RenderState.orderingFlag != 0,
            otDepth < 0xD00 ? RAGE_GAME_CAR_RENDER_CLOSE
                            : RAGE_GAME_CAR_RENDER_FAR);
    }
    obj->y -= g_TrackRenderTable->models[model].horizon;
    obj->modelY -= g_TrackRenderTable->models[model].horizon;
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
            MulMatrix2((&g_RenderState.matrix), &m_30);

            BuildRotMatrixY(&m_10, 0x800 - obj->modelYaw);
            BuildRotMatrixX(&m_70, obj->modelPitch);
            MulMatrix2(&m_10, &m_70);
            MulMatrix2((&g_RenderState.matrix), &m_70);
            BuildRotMatrixZ(&m_10, obj->modelRoll);
            MulMatrix2(&m_70, &m_10);

            v_138[0] = obj->x;
            v_138[2] = obj->z;
            v_138[1] = obj->modelY;
            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(v_138), &m_10);
            g_ScratchRenderMode = 0;
            SubmitModel((&g_RenderState),
                            (lod[0] + 1 < g_ModelBankCount) ? (lod[0] + 1) : 1);

            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(v_138), &m_10);
            g_ScratchRenderMode = 0;
            SubmitModel((&g_RenderState),
                            (lod[0] + 1 < g_ModelBankCount) ? (lod[0] + 1) : 1);

            BuildRotMatrixZ(&m_70, obj->bodyRoll);
            MulMatrix2(&m_30, &m_70);
            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_70);
            g_ScratchRenderMode = lod[1] << 16;
            SubmitModel((&g_RenderState),
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
            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_F0);
            g_ScratchRenderMode = 0;
            SubmitModel((&g_RenderState),
                            (lod[0] + 3 < g_ModelBankCount) ? (lod[0] + 3) : 1);

            for (i = 0; i < 2; i++) {
                s32 ax = g_TrackRenderTable->models[model].axis0;
                if (i % 2) {
                    ax = -ax;
                }
                v_110[0] = ax;
                v_110[1] = g_TrackRenderTable->models[model].axis1;
                v_110[2] = g_TrackRenderTable->models[model].axis2;
                ApplyMatrix(&m_50, v_110, m_118);
                m_118[0] += obj->x;
                m_118[1] += obj->y;
                m_118[2] += obj->z;
                SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(m_118), &m_B0[i]);
                g_ScratchRenderMode = 0;
                SubmitModel((&g_RenderState),
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
            MulMatrix2((&g_RenderState.matrix), &m_10);
            SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_10);
            g_ScratchRenderMode = lod[1] << 16;
            SubmitModel((&g_RenderState),
                            (lod[0] + 4 < g_ModelBankCount) ? (lod[0] + 4) : 1);
        }
    }

    obj->y += g_TrackRenderTable->models[model].horizon;
    obj->modelY += g_TrackRenderTable->models[model].horizon;
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}
