#include "game/state.h"
#include "game/input_internal.h"
#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"

/* Loads the GTE light matrix with g_SceneLightMatrix * `view`, working on a
 * local copy so the caller's view matrix is left alone. */
static void SetGteLightMatrix(Matrix *view) {
    Matrix m;

    m = *view;
    MulMatrix2(&g_SceneLightMatrix, &m);
    SetLightMatrix(&m);
}

/* Controller geometry is the foreground layer of OPTION. Submit it through
 * the secondary table after the primary backdrop; its natural model depth
 * then keeps it behind the secondary-table callouts and labels. */
static void SubmitControllerModel(s32 model) {
    GameOrderingTableEntry *otBase = RENDER_OT_BASE;
    GameFrameContext *frame = g_DrawBuffer;

    RENDER_OT_BASE = frame->layout.orderingTables[1];
    SubmitModel((&g_RenderState), model);
    RENDER_OT_BASE = otBase;
}

static void BuildControllerPartTransform(Matrix *transform, s32 pitch) {
    Matrix yawRotation;
    s32 scale[3] = {0x1000, 0x2000, 0x1000};

    BuildRotMatrixX(transform, pitch);
    BuildRotMatrixY(&yawRotation, g_ControllerSceneAngleY + 0x400);
    MulMatrix2(&yawRotation, transform);
    MulMatrix2(&g_RenderState.matrix, transform);
    ScaleMatrix(&yawRotation, scale);
    MulMatrix2(&yawRotation, transform);
    SetGteLightMatrix(transform);
}

static void SubmitControllerPart(LVec *position, Matrix *transform, s32 model) {
    SetGteObjectMatrix(&g_ObjectMatrixWork, position, transform);
    g_RenderState.envMode4 = 0;
    SubmitControllerModel(model);
}

static s32 ControllerModelOrFallback(s32 model, s32 requiredModelCount) {
    return g_ModelBankCount >= requiredModelCount ? model : 1;
}

/* Builds and submits the controller models shown by the pad and NeGcon setup
 * screens. The read-only render state is retained for the three camera
 * matrix multiplies; write-only fields stay absolute so each store
 * is independently rematerialized. */
void DrawControllerSetupScene(s32 showButtonOverlays) {
    Matrix partTransform;
    LVec position = {0, 0, 0};
    s32 baseAngle;
    s32 steer;
    s32 model;
    s32 isCalibrationGauge;

    g_RenderState.viewZ = -0x1080;
    g_RenderState.viewY = 0;
    g_RenderState.viewX = 0;
    g_RenderState.viewAngleZ = 0;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleX = 0;
    isCalibrationGauge =
        g_GameMode == OPTION_MODE_NEGCON_STEER_PLAY ||
        g_GameMode == OPTION_MODE_NEGCON_MAX_TWIST;
    if (isCalibrationGauge) {
        g_RenderState.viewZ = -0xC80;
    } else {
        g_RenderState.viewY = -0x40;
    }
    SetCameraRotMatrix();

    if (g_PadType == PAD_TYPE_DIGITAL) {
        BuildControllerPartTransform(&partTransform, -0xD0);
        model = ControllerModelOrFallback(0, 1);
        SubmitControllerPart(&position, &partTransform, model);
        return;
    }

    if (g_PadType != PAD_TYPE_NEGCON) {
        return;
    }
    if (g_GameMode == OPTION_MODE_NEGCON_MAX_TWIST) {
        steer = (rsin(g_AnimTimer * 16) * GetNegconSteerRange()) / 512;
    } else if (g_GameMode == OPTION_MODE_NEGCON_STEER_PLAY) {
        steer = ((rsin(g_AnimTimer * 16) * 16) *
                 g_NegconPlayScale[g_NegconSteerPlay]) / 4096;
    } else {
        steer = g_NegconSteer * 8;
    }

    baseAngle = g_ControllerSceneAngleX - 0x40;
    BuildControllerPartTransform(&partTransform, baseAngle + steer);
    SubmitControllerPart(&position, &partTransform, 1);
    if (showButtonOverlays) {
        model = ControllerModelOrFallback(3, 4);
        SubmitControllerPart(&position, &partTransform, model);
    }

    BuildControllerPartTransform(&partTransform, baseAngle - steer);
    model = ControllerModelOrFallback(2, 3);
    SubmitControllerPart(&position, &partTransform, model);
    if (showButtonOverlays) {
        model = ControllerModelOrFallback(4, 5);
        SubmitControllerPart(&position, &partTransform, model);
    }
}
