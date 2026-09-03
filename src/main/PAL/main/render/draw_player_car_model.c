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
    SetGteObjectMatrix(position, transform);
    g_RenderState.envMode4 = materialMode;
    SubmitModel(&g_RenderState, modelBank);
}

static void OffsetCarHorizon(GameRenderObject *object, s32 offset) {
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
} CloseCarAssembly;

/* Player and close rival cars use the same six-part matrix stack. Their model
 * banks, wheel geometry and steering scale come from different asset formats. */
static s32 DrawCloseCarAssembly(GameRenderObject *object,
                                const CloseCarAssembly *assembly) {
    Matrix scratchMatrix;
    Matrix bodyViewMatrix;
    Matrix bodyLocalMatrix;
    Matrix partMatrix;
    Matrix lightMatrix;
    Matrix wheelMatrices[CAR_SIDE_COUNT];
    Matrix axleMatrix;
    s16 wheelOffset[4];
    s32 wheelPosition[4];
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
        clipHandle = GetTrackZoneBlend(object->trackProgress);
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, &lightMatrix);
        }
    }
    SetLightMatrix(&lightMatrix);
    FlipMatrixXZColumns(&lightMatrix, &lightMatrix);

    bodyLocalMatrix = bodyViewMatrix;
    MulMatrix2(&g_RenderState.matrix, &bodyViewMatrix);

    BuildRotMatrixY(
        &scratchMatrix,
        WrapSigned32((int64_t)ANGLE_HALF_TURN - object->modelYaw));
    BuildRotMatrixX(&partMatrix, object->modelPitch);
    MulMatrix2(&scratchMatrix, &partMatrix);
    MulMatrix2(&g_RenderState.matrix, &partMatrix);
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
    SubmitCarPart(AsPositionWords(&object->x), &partMatrix,
                  assembly->bodyMaterialMode, assembly->bodyBank);

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

        wheelOffset[0] = WrapSigned16(lateralOffset);
        wheelOffset[1] = assembly->wheelOffsetY;
        wheelOffset[2] = assembly->wheelOffsetZ;
        ApplyMatrix(&bodyLocalMatrix, wheelOffset, wheelPosition);
        wheelPosition[0] = WrapSigned32(
            (int64_t)wheelPosition[0] + object->x);
        wheelPosition[1] = WrapSigned32(
            (int64_t)wheelPosition[1] + object->y);
        wheelPosition[2] = WrapSigned32(
            (int64_t)wheelPosition[2] + object->z);
        SubmitCarPart(AsPositionWords(wheelPosition),
                      &wheelMatrices[sideIndex], 0,
                      assembly->wheelBank);
        SetLightMatrix(&lightMatrix);
    }

    return clipHandle;
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
    s32 modelBankBase = WrapSigned32(
        (int64_t)object->renderDepth * 2);
    s32 clipHandle;

    GameRenderWorldSubmitPlayerCar(object, g_RenderState.orderingFlag != 0);

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
    };

    OffsetCarHorizon(object, -modelAsset->horizon);
    clipHandle = DrawCloseCarAssembly(object, &assembly);
    OffsetCarHorizon(object, modelAsset->horizon);
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}

void DrawCar(GameRenderObject *object) {
    Matrix scratchMatrix;
    Matrix bodyLocalMatrix;
    Matrix lightMatrix;
    s32 cameraOffset[4];
    s32 viewPosition[4];
    s32 clipHandle = 0;
    s32 renderDistance;
    s32 model;
    s32 horizon;
    s16 *lod;
    CarRenderRange renderRange;

    model = g_CarModelByCourse[SeriesCourseIndex()][object->modelIndex];
    lod = g_CarModelBankTable[model];

    cameraOffset[0] = WrapSigned32(
        (int64_t)object->x - g_RenderState.viewX);
    cameraOffset[1] = 0;
    cameraOffset[2] = WrapSigned32(
        (int64_t)object->z - g_RenderState.viewZ);
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
            .useZoneLighting = 1,
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
        clipHandle = GetTrackZoneBlend(object->trackProgress);
        if (clipHandle != 0) {
            ApplyZoneLighting(clipHandle, &lightMatrix);
        }
        SetLightMatrix(&lightMatrix);

        BuildRotMatrixZ(&scratchMatrix, object->bodyRoll);
        MulMatrix2(&bodyLocalMatrix, &scratchMatrix);
        MulMatrix2(&g_RenderState.matrix, &scratchMatrix);
        SubmitCarPart(AsPositionWords(&object->x), &scratchMatrix,
                      CarMaterialMode(lod[1]),
                      ResolveCarModelBank(lod[0], 4, g_ModelBankCount));
    }

    OffsetCarHorizon(object, horizon);
    if (clipHandle != 0) {
        RestoreColorMatrix();
    }
}
