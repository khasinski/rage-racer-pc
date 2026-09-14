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
#include "game/track.h"
#include "rage/render_world_game.h"

enum {
    CAR_SHELL_PASS_COUNT = 2,
    CAR_SIDE_COUNT = 2,
};

static void SubmitCarPart(const LVec *position, Matrix *transform,
                          s32 materialMode, s32 modelBank) {
    SetGteObjectMatrix(position, transform);
    g_RenderState.geometry.envMode4 = materialMode;
    SubmitModel(&g_RenderState, modelBank);
}

static void OffsetCarHorizon(GameCarRuntime *object, s32 offset) {
    object->y = WrapSigned32((int64_t)object->y + offset);
    object->modelY = WrapSigned32((int64_t)object->modelY + offset);
}

static s32 CarMaterialMode(s16 palette) {
    return (s32)((u32)(u16)palette << 16);
}

typedef struct CloseCarAssembly {
    s32 shellBank;
    s32 bodyBank;
    s32 axleBank;
    s32 wheelBank;
    s32 bodyMaterialMode;
    s32 steeringAngle;
    s16 wheelOffsetX;
    s16 wheelOffsetY;
    s16 wheelOffsetZ;
    s32 useZoneLighting;
    s32 bodyUsesAuxiliaryTextures;
} CloseCarAssembly;

static s32 s_previewUsesAuxiliaryTextures;

/* Player and close rival cars use the same six-part matrix stack. Their model
 * banks, wheel geometry and steering scale come from different asset formats. */
static s32 DrawCloseCarAssembly(GameCarRuntime *object,
                                const CloseCarAssembly *assembly) {
    Matrix scratchMatrix;
    Matrix bodyViewMatrix;
    Matrix bodyLocalMatrix;
    Matrix partMatrix;
    Matrix lightMatrix;
    Matrix wheelMatrices[CAR_SIDE_COUNT];
    Matrix axleMatrix;
    SVec wheelOffset = {0};
    Vec4 wheelPosition = {0};
    LVec modelPosition;
    s32 clipHandle = 0;
    s32 passIndex;
    s32 sideIndex;

    BuildRotMatrixY(
        &scratchMatrix,
        WrapSigned32((int64_t)ANGLE_HALF_TURN - object->bodyYaw));
    BuildRotMatrixX(&bodyViewMatrix, object->bodyPitch);
    MulMatrix2(&scratchMatrix, &bodyViewMatrix);
    MulMatrix0(&g_SceneLightMatrix, &bodyViewMatrix, &lightMatrix);

    if (assembly->useZoneLighting) {
        TrackZoneEffect zone = GetTrackZoneEffect(object->trackProgress);

        clipHandle = zone.blend;
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, zone.code, &lightMatrix);
        }
    }
    SetLightMatrix(&lightMatrix);
    FlipMatrixXZColumns(&lightMatrix, &lightMatrix);

    bodyLocalMatrix = bodyViewMatrix;
    MulMatrix2(&g_RenderState.geometry.matrix, &bodyViewMatrix);

    BuildRotMatrixY(
        &scratchMatrix,
        WrapSigned32((int64_t)ANGLE_HALF_TURN - object->modelYaw));
    BuildRotMatrixX(&partMatrix, object->modelPitch);
    MulMatrix2(&scratchMatrix, &partMatrix);
    MulMatrix2(&g_RenderState.geometry.matrix, &partMatrix);
    BuildRotMatrixZ(&scratchMatrix, object->modelRoll);
    MulMatrix2(&partMatrix, &scratchMatrix);

    modelPosition.x = object->x;
    modelPosition.y = object->modelY;
    modelPosition.z = object->z;
    /* The recovered retail path submits this shell twice with one bank. */
    for (passIndex = 0; passIndex < CAR_SHELL_PASS_COUNT; passIndex++) {
        SubmitCarPart(&modelPosition, &scratchMatrix, 0,
                      assembly->shellBank);
    }

    BuildRotMatrixZ(&partMatrix, object->bodyRoll);
    MulMatrix2(&bodyViewMatrix, &partMatrix);
    UseAuxiliaryModelTextures(assembly->bodyUsesAuxiliaryTextures);
    SubmitCarPart(AsPositionWords(&object->x), &partMatrix,
                  assembly->bodyMaterialMode, assembly->bodyBank);
    UseAuxiliaryModelTextures(0);

    BuildRotMatrixZ(
        &scratchMatrix,
        WrapSigned32(
            (int64_t)object->bodyRoll - object->bodyRollVelocity));
    MulMatrix(&bodyLocalMatrix, &scratchMatrix);
    MulMatrix(&bodyViewMatrix, &scratchMatrix);
    BuildRotMatrixX(&axleMatrix, object->wheelRotation);
    MulMatrix2(&bodyViewMatrix, &axleMatrix);

    BuildRotMatrixY(&scratchMatrix, assembly->steeringAngle);
    BuildRotMatrixX(&wheelMatrices[0], object->wheelRotation);
    MulMatrix2(&scratchMatrix, &wheelMatrices[0]);
    MulMatrix2(&bodyViewMatrix, &wheelMatrices[0]);

    FlipMatrixXZColumns(&wheelMatrices[1], &wheelMatrices[0]);
    SubmitCarPart(AsPositionWords(&object->x), &axleMatrix, 0,
                  assembly->axleBank);

    for (sideIndex = 0; sideIndex < CAR_SIDE_COUNT; sideIndex++) {
        const int64_t lateralOffset = sideIndex == 0
            ? assembly->wheelOffsetX
            : -(int64_t)assembly->wheelOffsetX;

        wheelOffset.vx = WrapSigned16(lateralOffset);
        wheelOffset.vy = assembly->wheelOffsetY;
        wheelOffset.vz = assembly->wheelOffsetZ;
        ApplyMatrix(&bodyLocalMatrix, &wheelOffset, &wheelPosition);
        wheelPosition.x = WrapSigned32(
            (int64_t)wheelPosition.x + object->x);
        wheelPosition.y = WrapSigned32(
            (int64_t)wheelPosition.y + object->y);
        wheelPosition.z = WrapSigned32(
            (int64_t)wheelPosition.z + object->z);
        SubmitCarPart(AsPosition(&wheelPosition),
                      &wheelMatrices[sideIndex], 0,
                      assembly->wheelBank);
        SetLightMatrix(&lightMatrix);
    }

    return clipHandle;
}

/*
 * GameCarRuntime -> GPU-primitive submitter. Applies the model asset's
 * horizon offset, builds a stack of rotation matrices from the
 * object's angle sets, loads each transform into the GTE and dispatches the
 * primitive builder SubmitModel at increasing depth buckets. X/Z column flips
 * build the opposite-side transforms; clipHandle is the optional lighting
 * volume from GetTrackZoneEffect.
 */
void DrawPlayerCarModel(GameCarRuntime *object) {
    const CarModelAsset *modelAsset = g_CarModelAsset;
    s32 modelBankBase = WrapSigned32(
        (int64_t)object->renderDepth * 2);
    s32 clipHandle;

    GameRenderWorldSubmitPlayerCar(object, g_RenderState.pass.orderingFlag != 0);

    if (object->wheelRotation & CAR_WHEEL_BLUR_FLAG) {
        modelBankBase = WrapSigned32((int64_t)modelBankBase + 10);
    }

    const CloseCarAssembly assembly = {
        .shellBank = 1,
        .bodyBank = ResolveCarModelBank(0, 0, g_ModelBankCount),
        .axleBank = ResolveCarModelBank(
            modelBankBase, 3, g_ModelBankCount),
        .wheelBank = ResolveCarModelBank(
            modelBankBase, 2, g_ModelBankCount),
        .bodyMaterialMode = 0,
        .steeringAngle = object->steeringAngle / 12,
        .wheelOffsetX = modelAsset->modelOffsetX,
        .wheelOffsetY = modelAsset->modelOffsetY,
        .wheelOffsetZ = modelAsset->modelOffsetZ,
        .useZoneLighting = g_SceneId != 8,
        .bodyUsesAuxiliaryTextures = 0,
    };

    OffsetCarHorizon(object, -modelAsset->horizon);
    clipHandle = DrawCloseCarAssembly(object, &assembly);
    OffsetCarHorizon(object, modelAsset->horizon);
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}

void DrawRacePlayerCarModel(GameCarRuntime *object) {
    s32 model = CustomRaceRivalModel();

    if (model >= 0) {
        s32 savedModel = object->modelIndex;
        object->modelIndex = (s16)model;
        DrawCar(object);
        object->modelIndex = (s16)savedModel;
        return;
    }
    DrawPlayerCarModel(object);
}

void DrawCustomRivalPreview(GameCarRuntime *object, s32 selection) {
    const TrackRenderTable *previewTable;
    const TrackRenderTable *savedTable;
    s32 savedModel;

    previewTable = CustomRivalPreviewRenderTable();
    s32 rival = CustomRaceRivalModelForSelection(selection);
    if (object == NULL || rival < 0 || previewTable == NULL)
        return;
    savedModel = object->modelIndex;
    savedTable = g_TrackRenderTable;
    object->modelIndex = (s16)rival;
    g_TrackRenderTable = previewTable;
    SelectModelBank(13);
    s_previewUsesAuxiliaryTextures = 1;
    DrawCar(object);
    s_previewUsesAuxiliaryTextures = 0;
    g_TrackRenderTable = savedTable;
    object->modelIndex = (s16)savedModel;
}

void DrawCar(GameCarRuntime *object) {
    Matrix scratchMatrix;
    Matrix bodyLocalMatrix;
    Matrix lightMatrix;
    Vec4 cameraOffset = {0};
    Vec4 viewPosition = {0};
    s32 clipHandle = 0;
    s32 renderDistance;
    s32 model;
    s32 horizon;
    s16 *lod;
    CarRenderRange renderRange;

    model = g_CarModelByCourse[SeriesCourseIndex()][object->modelIndex];
    lod = g_CarModelBankTable[model];

    cameraOffset.x = WrapSigned32(
        (int64_t)object->x - g_Camera.view.x);
    cameraOffset.z = WrapSigned32(
        (int64_t)object->z - g_Camera.view.z);
    ApplyMatrixLV(&g_RenderState.geometry.matrix, AsWords(&cameraOffset),
                  AsWords(&viewPosition));
    renderDistance = CarRenderManhattanDistance(
        object->x, object->z, g_Camera.view.x, g_Camera.view.z);
    renderRange = ClassifyCarRenderRange(viewPosition.z, renderDistance);
    if (renderRange == CAR_RENDER_CLOSE || renderRange == CAR_RENDER_FAR) {
        GameRenderWorldSubmitCar(
            object, g_RenderState.pass.orderingFlag != 0,
            renderRange == CAR_RENDER_CLOSE ? RAGE_GAME_CAR_RENDER_CLOSE
                                            : RAGE_GAME_CAR_RENDER_FAR);
    } else {
        return;
    }

    horizon = g_TrackRenderTable->models[model].horizon;
    OffsetCarHorizon(object, -horizon);
    if (renderRange == CAR_RENDER_CLOSE) {
        const CarModelRenderParams *params =
            &g_TrackRenderTable->models[model];
        const CloseCarAssembly assembly = {
            .shellBank = ResolveCarModelBank(
                lod[0], 1, g_ModelBankCount),
            .bodyBank = ResolveCarModelBank(
                lod[0], 0, g_ModelBankCount),
            .axleBank = ResolveCarModelBank(
                lod[0], 3, g_ModelBankCount),
            .wheelBank = ResolveCarModelBank(
                lod[0], 2, g_ModelBankCount),
            .bodyMaterialMode = CarMaterialMode(lod[1]),
            .steeringAngle = WrapSigned32(
                (int64_t)object->steeringAngle * 2),
            .wheelOffsetX = WrapSigned16(params->axis0),
            .wheelOffsetY = WrapSigned16(params->axis1),
            .wheelOffsetZ = WrapSigned16(params->axis2),
            .useZoneLighting = g_SceneId != 8,
            .bodyUsesAuxiliaryTextures = s_previewUsesAuxiliaryTextures,
        };

        clipHandle = DrawCloseCarAssembly(object, &assembly);
    } else {
        BuildRotMatrixY(
            &scratchMatrix,
            WrapSigned32(
                (int64_t)ANGLE_HALF_TURN - object->bodyYaw));
        BuildRotMatrixX(&bodyLocalMatrix, object->bodyPitch);
        MulMatrix2(&scratchMatrix, &bodyLocalMatrix);
        MulMatrix0(&g_SceneLightMatrix, &bodyLocalMatrix, &lightMatrix);
        TrackZoneEffect zone = GetTrackZoneEffect(object->trackProgress);

        clipHandle = zone.blend;
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, zone.code, &lightMatrix);
        }
        SetLightMatrix(&lightMatrix);

        BuildRotMatrixZ(&scratchMatrix, object->bodyRoll);
        MulMatrix2(&bodyLocalMatrix, &scratchMatrix);
        MulMatrix2(&g_RenderState.geometry.matrix, &scratchMatrix);
        SubmitCarPart(AsPositionWords(&object->x), &scratchMatrix,
                      CarMaterialMode(lod[1]),
                      ResolveCarModelBank(lod[0], 4, g_ModelBankCount));
    }

    OffsetCarHorizon(object, horizon);
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}
