#include "game/diagnostics.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/car_render_rules.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "rage/render_world_game.h"

#include <stdio.h>
#include <stdlib.h>

static s32 FindRenderedCarSlot(const GameRenderObject *object) {
    s32 slot;

    if (object == GetCarRenderObject(AsRivalCar(&g_PlayerCar))) {
        return -1;
    }
    for (slot = 0; slot < RACE_CAR_SLOT_COUNT; slot++) {
        if (object == GetCarRenderObject(&g_Cars[slot])) {
            return slot;
        }
    }
    return -2;
}

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
    g_RenderState.envMode4 = 0;
    SubmitModel((&g_RenderState), 1);

    SetGteObjectMatrix((&g_ObjectMatrixWork), &modelPosition, &m_10);
    g_RenderState.envMode4 = 0;
    SubmitModel((&g_RenderState), 1);

    BuildRotMatrixZ(&m_70, obj->bodyRoll);
    MulMatrix2(&m_30, &m_70);
    SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_70);
    g_RenderState.envMode4 = 0;
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
    g_RenderState.envMode4 = 0;
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
        g_RenderState.envMode4 = 0;
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
    s16 wheelOffset[4];
    s32 wheelPosition[4];
    s32 cameraOffset[4];
    s32 modelPosition[4];
    s32 viewPosition[4];
    s32 clipHandle = 0;
    s32 renderDistance;
    s32 i;
    s32 model;
    s16 *lod;
    CarRenderRange renderRange;

    model = g_CarModelByCourse[SeriesCourseIndex()][obj->modelIndex];
    lod = g_CarModelBankTable[model];

    cameraOffset[0] = obj->x - g_RenderState.viewX;
    cameraOffset[1] = 0;
    cameraOffset[2] = obj->z - g_RenderState.viewZ;
    ApplyMatrixLV(&g_RenderState.matrix, cameraOffset, viewPosition);
    renderDistance = CarRenderManhattanDistance(
        obj->x, obj->z, g_RenderState.viewX, g_RenderState.viewZ);
    renderRange = ClassifyCarRenderRange(viewPosition[2], renderDistance);
    if (DiagnosticsEnabled("render.car_draw_trace")) {
        const char *timerText = DiagnosticsValue("render.car_draw_trace_timer");
        if (timerText == NULL || g_SceneTimer == (s32)strtol(timerText, NULL, 0)) {
            static const char *const rangeNames[] = {
                "behind", "close", "far", "culled"
            };
            Trace("car-draw", "timer=%d mirror=%d slot=%d source=%d "
                   "car=%d lod=%d palette=%d depth=%d view-z=%d detail=%s "
                   "player=%d grade=%d asset=%d",
                   g_SceneTimer, g_RenderState.orderingFlag != 0,
                   FindRenderedCarSlot(obj),
                   obj->modelIndex, model, lod[0], lod[1], renderDistance,
                   viewPosition[2], rangeNames[renderRange], g_PlayerCarIndex,
                   g_CarTable[g_PlayerCarIndex].modelVariant,
                   GetCarAssetIndex(g_PlayerCarIndex,
                       g_CarTable[g_PlayerCarIndex].modelVariant));
        }
    }
    if (renderRange == CAR_RENDER_CLOSE || renderRange == CAR_RENDER_FAR) {
        GameRenderWorldSubmitCar(
            obj, g_RenderState.orderingFlag != 0,
            renderRange == CAR_RENDER_CLOSE ? RAGE_GAME_CAR_RENDER_CLOSE
                                            : RAGE_GAME_CAR_RENDER_FAR);
    } else {
        return;
    }

    obj->y -= g_TrackRenderTable->models[model].horizon;
    obj->modelY -= g_TrackRenderTable->models[model].horizon;
    if (renderRange == CAR_RENDER_CLOSE) {
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

        modelPosition[0] = obj->x;
        modelPosition[2] = obj->z;
        modelPosition[1] = obj->modelY;
        SetGteObjectMatrix(&g_ObjectMatrixWork,
                           AsPositionWords(modelPosition), &m_10);
        g_RenderState.envMode4 = 0;
        SubmitModel((&g_RenderState),
                    ResolveCarModelBank(lod[0], 1, g_ModelBankCount));

        SetGteObjectMatrix(&g_ObjectMatrixWork,
                           AsPositionWords(modelPosition), &m_10);
        g_RenderState.envMode4 = 0;
        SubmitModel((&g_RenderState),
                    ResolveCarModelBank(lod[0], 1, g_ModelBankCount));

        BuildRotMatrixZ(&m_70, obj->bodyRoll);
        MulMatrix2(&m_30, &m_70);
        SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&obj->x), &m_70);
        g_RenderState.envMode4 = lod[1] << 16;
        SubmitModel((&g_RenderState),
                    ResolveCarModelBank(lod[0], 0, g_ModelBankCount));

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
        g_RenderState.envMode4 = 0;
        SubmitModel((&g_RenderState),
                    ResolveCarModelBank(lod[0], 3, g_ModelBankCount));

        for (i = 0; i < 2; i++) {
            s32 ax = g_TrackRenderTable->models[model].axis0;
            if (i % 2) {
                ax = -ax;
            }
            wheelOffset[0] = ax;
            wheelOffset[1] = g_TrackRenderTable->models[model].axis1;
            wheelOffset[2] = g_TrackRenderTable->models[model].axis2;
            ApplyMatrix(&m_50, wheelOffset, wheelPosition);
            wheelPosition[0] += obj->x;
            wheelPosition[1] += obj->y;
            wheelPosition[2] += obj->z;
            SetGteObjectMatrix(&g_ObjectMatrixWork,
                               AsPositionWords(wheelPosition), &m_B0[i]);
            g_RenderState.envMode4 = 0;
            SubmitModel((&g_RenderState),
                        ResolveCarModelBank(lod[0], 2,
                                           g_ModelBankCount));
            SetLightMatrix(&m_90);
        }
    } else {
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
        g_RenderState.envMode4 = lod[1] << 16;
        SubmitModel((&g_RenderState),
                    ResolveCarModelBank(lod[0], 4, g_ModelBankCount));
    }

    obj->y += g_TrackRenderTable->models[model].horizon;
    obj->modelY += g_TrackRenderTable->models[model].horizon;
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}
