#include "game/diagnostics.h"
#include "game/angle.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/car_model_matrix.h"
#include "game/car_render_rules.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "rage/render_world_game.h"

enum {
    CAR_SHELL_PASS_COUNT = 2,
    CAR_SIDE_COUNT = 2,
};

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

static void SubmitCarPart(LVec *position, Matrix *transform, s32 materialMode,
                          s32 modelBank) {
    SetGteObjectMatrix(&g_ObjectMatrixWork, position, transform);
    g_RenderState.envMode4 = materialMode;
    SubmitModel(&g_RenderState, modelBank);
}

static void OffsetCarHorizon(GameRenderObject *object, s32 offset) {
    object->y += offset;
    object->modelY += offset;
}

/*
 * GameRenderObject -> GPU-primitive submitter. Applies the model asset's
 * horizon offset, builds a stack of rotation matrices from the
 * object's angle sets, loads each transform into the GTE and dispatches the
 * primitive builder SubmitModel at increasing depth buckets. X/Z column flips
 * build the opposite-side transforms; clipHandle is the optional lighting
 * volume from GetTrackZoneBlend.
 */
void DrawPlayerCarModel(GameRenderObject *object) {
    const CarModelAsset *modelAsset = g_CarModelAsset;
    Matrix scratchMatrix;
    Matrix bodyViewMatrix;
    Matrix bodyLocalMatrix;
    Matrix partMatrix;
    Matrix lightMatrix;
    Matrix wheelMatrices[2];
    Matrix axleMatrix;
    s16 wheelOffset[4];
    s32 wheelPosition[4];
    LVec modelPosition;
    s32 clipHandle = 0;
    s32 modelBankBase;
    s32 passIndex;
    s32 sideIndex;

    GameRenderWorldSubmitPlayerCar(object, g_RenderState.orderingFlag != 0);

    OffsetCarHorizon(object, -modelAsset->horizon);
    BuildRotMatrixY(&scratchMatrix, ANGLE_HALF_TURN - object->bodyYaw);
    BuildRotMatrixX(&bodyViewMatrix, object->bodyPitch);
    MulMatrix2(&scratchMatrix, &bodyViewMatrix);
    MulMatrix0(&g_SceneLightMatrix, &bodyViewMatrix, &lightMatrix);

    if (g_SceneId != 8) {
        clipHandle = GetTrackZoneBlend(object->trackProgress);
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, &lightMatrix);
        }
    }
    SetLightMatrix(&lightMatrix);

    FlipMatrixXZColumns(&lightMatrix, &lightMatrix);

    bodyLocalMatrix = bodyViewMatrix;
    MulMatrix2(&g_RenderState.matrix, &bodyViewMatrix);

    BuildRotMatrixY(&scratchMatrix, ANGLE_HALF_TURN - object->modelYaw);
    BuildRotMatrixX(&partMatrix, object->modelPitch);
    MulMatrix2(&scratchMatrix, &partMatrix);
    MulMatrix2(&g_RenderState.matrix, &partMatrix);
    BuildRotMatrixZ(&scratchMatrix, object->modelRoll);
    MulMatrix2(&partMatrix, &scratchMatrix);

    modelPosition.x = object->x;
    modelPosition.z = object->z;
    modelPosition.y = object->modelY;
    /* The recovered retail path submits this shell twice with the same bank. */
    for (passIndex = 0; passIndex < CAR_SHELL_PASS_COUNT; passIndex++) {
        SubmitCarPart(&modelPosition, &scratchMatrix, 0, 1);
    }

    BuildRotMatrixZ(&partMatrix, object->bodyRoll);
    MulMatrix2(&bodyViewMatrix, &partMatrix);
    SubmitCarPart(AsPositionWords(&object->x), &partMatrix, 0,
                  ResolveCarModelBank(0, 0, g_ModelBankCount));

    modelBankBase = object->renderDepth * 2;
    if (object->wheelRotation & CAR_WHEEL_BLUR_FLAG) {
        modelBankBase += 10;
    }
    BuildRotMatrixZ(&scratchMatrix, object->bodyRoll - object->bodyRollVelocity);
    MulMatrix(&bodyLocalMatrix, &scratchMatrix);
    MulMatrix(&bodyViewMatrix, &scratchMatrix);
    BuildRotMatrixX(&axleMatrix, object->wheelRotation);
    MulMatrix2(&bodyViewMatrix, &axleMatrix);

    BuildRotMatrixY(&scratchMatrix, object->steeringAngle / 12);
    BuildRotMatrixX(&wheelMatrices[0], object->wheelRotation);
    MulMatrix2(&scratchMatrix, &wheelMatrices[0]);
    MulMatrix2(&bodyViewMatrix, &wheelMatrices[0]);

    FlipMatrixXZColumns(&wheelMatrices[1], &wheelMatrices[0]);
    SubmitCarPart(AsPositionWords(&object->x), &axleMatrix, 0,
                  ResolveCarModelBank(modelBankBase, 3, g_ModelBankCount));

    for (sideIndex = 0; sideIndex < CAR_SIDE_COUNT; sideIndex++) {
        s32 lateralOffset = modelAsset->modelOffsetX;
        if (sideIndex != 0) {
            lateralOffset = -lateralOffset;
        }
        wheelOffset[0] = lateralOffset;
        wheelOffset[1] = modelAsset->modelOffsetY;
        wheelOffset[2] = modelAsset->modelOffsetZ;
        ApplyMatrix(&bodyLocalMatrix, wheelOffset, wheelPosition);
        wheelPosition[0] += object->x;
        wheelPosition[1] += object->y;
        wheelPosition[2] += object->z;
        SubmitCarPart(AsPositionWords(wheelPosition),
                      &wheelMatrices[sideIndex], 0,
                      ResolveCarModelBank(modelBankBase, 2,
                                          g_ModelBankCount));
        SetLightMatrix(&lightMatrix);
    }

    OffsetCarHorizon(object, modelAsset->horizon);
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}

void DrawCar(GameRenderObject *object) {
    Matrix scratchMatrix;
    Matrix bodyViewMatrix;
    Matrix bodyLocalMatrix;
    Matrix partMatrix;
    Matrix lightMatrix;
    Matrix wheelMatrices[2];
    Matrix axleMatrix;
    s16 wheelOffset[4];
    s32 wheelPosition[4];
    s32 cameraOffset[4];
    s32 modelPosition[4];
    s32 viewPosition[4];
    s32 clipHandle = 0;
    s32 renderDistance;
    s32 passIndex;
    s32 sideIndex;
    s32 model;
    s32 horizon;
    s16 *lod;
    CarRenderRange renderRange;

    model = g_CarModelByCourse[SeriesCourseIndex()][object->modelIndex];
    lod = g_CarModelBankTable[model];

    cameraOffset[0] = object->x - g_RenderState.viewX;
    cameraOffset[1] = 0;
    cameraOffset[2] = object->z - g_RenderState.viewZ;
    ApplyMatrixLV(&g_RenderState.matrix, cameraOffset, viewPosition);
    renderDistance = CarRenderManhattanDistance(
        object->x, object->z, g_RenderState.viewX, g_RenderState.viewZ);
    renderRange = ClassifyCarRenderRange(viewPosition[2], renderDistance);
    if (DiagnosticsEnabled("render.car_draw_trace")) {
        if (g_SceneTimer == DiagnosticsIntValue(
                "render.car_draw_trace_timer", g_SceneTimer)) {
            static const char *const rangeNames[] = {
                "behind", "close", "far", "culled"
            };
            Trace("car-draw", "timer=%d mirror=%d slot=%d source=%d "
                   "car=%d lod=%d palette=%d depth=%d view-z=%d detail=%s "
                   "player=%d grade=%d asset=%d",
                   g_SceneTimer, g_RenderState.orderingFlag != 0,
                   FindRenderedCarSlot(object),
                   object->modelIndex, model, lod[0], lod[1], renderDistance,
                   viewPosition[2], rangeNames[renderRange], g_PlayerCarIndex,
                   g_CarTable[g_PlayerCarIndex].modelVariant,
                   GetCarAssetIndex(g_PlayerCarIndex,
                       g_CarTable[g_PlayerCarIndex].modelVariant));
        }
    }
    if (renderRange == CAR_RENDER_CLOSE || renderRange == CAR_RENDER_FAR) {
        GameRenderWorldSubmitCar(
            object, g_RenderState.orderingFlag != 0,
            renderRange == CAR_RENDER_CLOSE ? RAGE_GAME_CAR_RENDER_CLOSE
                                            : RAGE_GAME_CAR_RENDER_FAR);
    } else {
        return;
    }

    horizon = g_TrackRenderTable->models[model].horizon;
    OffsetCarHorizon(object, -horizon);
    if (renderRange == CAR_RENDER_CLOSE) {
        BuildRotMatrixY(&scratchMatrix, ANGLE_HALF_TURN - object->bodyYaw);
        BuildRotMatrixX(&bodyViewMatrix, object->bodyPitch);
        MulMatrix2(&scratchMatrix, &bodyViewMatrix);
        MulMatrix0(&g_SceneLightMatrix, &bodyViewMatrix, &lightMatrix);
        clipHandle = GetTrackZoneBlend(object->trackProgress);
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, &lightMatrix);
        }
        SetLightMatrix(&lightMatrix);

        FlipMatrixXZColumns(&lightMatrix, &lightMatrix);

        bodyLocalMatrix = bodyViewMatrix;
        MulMatrix2(&g_RenderState.matrix, &bodyViewMatrix);

        BuildRotMatrixY(&scratchMatrix, ANGLE_HALF_TURN - object->modelYaw);
        BuildRotMatrixX(&partMatrix, object->modelPitch);
        MulMatrix2(&scratchMatrix, &partMatrix);
        MulMatrix2(&g_RenderState.matrix, &partMatrix);
        BuildRotMatrixZ(&scratchMatrix, object->modelRoll);
        MulMatrix2(&partMatrix, &scratchMatrix);

        modelPosition[0] = object->x;
        modelPosition[2] = object->z;
        modelPosition[1] = object->modelY;
        /* The recovered retail path submits this shell twice with one bank. */
        for (passIndex = 0; passIndex < CAR_SHELL_PASS_COUNT; passIndex++) {
            SubmitCarPart(AsPositionWords(modelPosition), &scratchMatrix, 0,
                          ResolveCarModelBank(lod[0], 1,
                                              g_ModelBankCount));
        }

        BuildRotMatrixZ(&partMatrix, object->bodyRoll);
        MulMatrix2(&bodyViewMatrix, &partMatrix);
        SubmitCarPart(AsPositionWords(&object->x), &partMatrix, lod[1] << 16,
                      ResolveCarModelBank(lod[0], 0, g_ModelBankCount));

        BuildRotMatrixZ(&scratchMatrix, object->bodyRoll - object->bodyRollVelocity);
        MulMatrix(&bodyLocalMatrix, &scratchMatrix);
        MulMatrix(&bodyViewMatrix, &scratchMatrix);
        BuildRotMatrixX(&axleMatrix, object->wheelRotation);
        MulMatrix2(&bodyViewMatrix, &axleMatrix);

        BuildRotMatrixY(&scratchMatrix, object->steeringAngle * 2);
        BuildRotMatrixX(&wheelMatrices[0], object->wheelRotation);
        MulMatrix2(&scratchMatrix, &wheelMatrices[0]);
        MulMatrix2(&bodyViewMatrix, &wheelMatrices[0]);

        FlipMatrixXZColumns(&wheelMatrices[1], &wheelMatrices[0]);
        SubmitCarPart(AsPositionWords(&object->x), &axleMatrix, 0,
                      ResolveCarModelBank(lod[0], 3, g_ModelBankCount));

        for (sideIndex = 0; sideIndex < CAR_SIDE_COUNT; sideIndex++) {
            s32 lateralOffset = g_TrackRenderTable->models[model].axis0;
            if (sideIndex != 0) {
                lateralOffset = -lateralOffset;
            }
            wheelOffset[0] = lateralOffset;
            wheelOffset[1] = g_TrackRenderTable->models[model].axis1;
            wheelOffset[2] = g_TrackRenderTable->models[model].axis2;
            ApplyMatrix(&bodyLocalMatrix, wheelOffset, wheelPosition);
            wheelPosition[0] += object->x;
            wheelPosition[1] += object->y;
            wheelPosition[2] += object->z;
            SubmitCarPart(AsPositionWords(wheelPosition),
                          &wheelMatrices[sideIndex], 0,
                          ResolveCarModelBank(lod[0], 2,
                                              g_ModelBankCount));
            SetLightMatrix(&lightMatrix);
        }
    } else {
        BuildRotMatrixY(&scratchMatrix, ANGLE_HALF_TURN - object->bodyYaw);
        BuildRotMatrixX(&bodyLocalMatrix, object->bodyPitch);
        MulMatrix2(&scratchMatrix, &bodyLocalMatrix);
        MulMatrix0(&g_SceneLightMatrix, &bodyLocalMatrix, &lightMatrix);
        clipHandle = GetTrackZoneBlend(object->trackProgress);
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, &lightMatrix);
        }
        SetLightMatrix(&lightMatrix);

        BuildRotMatrixZ(&scratchMatrix, object->bodyRoll);
        MulMatrix2(&bodyLocalMatrix, &scratchMatrix);
        MulMatrix2(&g_RenderState.matrix, &scratchMatrix);
        SubmitCarPart(AsPositionWords(&object->x), &scratchMatrix,
                      lod[1] << 16,
                      ResolveCarModelBank(lod[0], 4, g_ModelBankCount));
    }

    OffsetCarHorizon(object, horizon);
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}
