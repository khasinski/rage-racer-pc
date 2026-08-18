#include "common.h"
#include "game/game_input.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/scratchpad_legacy.h"
#include "psyq/gte.h"

/* Loads the GTE light matrix with g_SceneLightMatrix * `view`, working on a
 * local copy so the caller's view matrix is left alone. */
void SetGteLightMatrix(Matrix *view) {
    Matrix m;

    m = *view;
    MulMatrix2(&g_SceneLightMatrix, &m);
    SetLightMatrix(&m);
}

/* Controller geometry is the foreground layer of OPTION. Submit it through
 * the secondary table after the primary backdrop; its natural model depth
 * then keeps it behind the secondary-table callouts and labels. */
static void SubmitControllerModel(s32 model) {
    OT_TYPE *otBase = RENDER_OT_BASE_AS(OT_TYPE);
    GameFrameContext *frame = GetGameFrameContext(g_DrawBuffer);

    RENDER_OT_BASE_AS(OT_TYPE) = frame->layout.orderingTables[1];
    SubmitModel(RENDER_WORKSPACE, model);
    RENDER_OT_BASE_AS(OT_TYPE) = otBase;
}

/* Builds and submits the controller models shown by the pad and NeGcon setup
 * screens. The read-only scratchpad base is retained for the three camera
 * matrix multiplies; write-only scratch locations stay absolute so each store
 * is independently rematerialized. */
void DrawControllerSetupScene(s32 variant) {
    s32 scale[3];
    Matrix xRot;
    Matrix yRot;
    s32 position[3];
    s32 steer;
    s32 model;
    u32 setupMode;

    RENDER_VIEW_Z = 0;
    RENDER_VIEW_Z = -0x1080;
    position[2] = 0;
    position[1] = 0;
    position[0] = 0;
    RENDER_VIEW_Y = 0;
    RENDER_VIEW_X = 0;
    RENDER_VIEW_ANGLE_Z = 0;
    RENDER_VIEW_ANGLE_Y = 0;
    RENDER_VIEW_ANGLE_X = 0;
    RENDER_VIEW_Y = 0;
    setupMode = g_GameMode - 10;
    if (setupMode < 2) {
        RENDER_VIEW_Z = -0xC80;
    } else {
        RENDER_VIEW_Y = -0x40;
    }
    SetCameraRotMatrix();

    if (g_GameInput.controllerType == 0x41) {
        BuildRotMatrixX(&xRot, -0xD0);
        BuildRotMatrixY(&yRot, g_ControllerSceneAngleY + 0x400);
        MulMatrix2(&yRot, &xRot);
        MulMatrix2(RENDER_VIEW_MATRIX_GTE, &xRot);
        scale[2] = 0x1000;
        scale[0] = 0x1000;
        scale[1] = 0x2000;
        ScaleMatrix(&yRot, scale);
        MulMatrix2(&yRot, &xRot);
        SetGteLightMatrix(&xRot);
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, position, &xRot);
        RENDER_ENV_MODE4 = 0;
        model = g_ModelBankCount < 1;
        SubmitControllerModel(model);
        return;
    }

    if (g_GameInput.controllerType != 0x23) {
        return;
    }
    if (g_GameMode == 11) {
        steer = (rsin(g_AnimTimer * 16) *
                 g_NegconSteerRange[g_NegconMaxTwist]) / 512;
    } else if (g_GameMode == 10) {
        steer = ((rsin(g_AnimTimer * 16) * 16) *
                 g_NegconPlayScale[g_NegconSteerPlay]) / 4096;
    } else {
        steer = g_GameInput.steering * 8;
    }

    {
        s32 angle = g_ControllerSceneAngleX - 0x40;

        BuildRotMatrixX(&xRot, steer + angle);
    }
    BuildRotMatrixY(&yRot, g_ControllerSceneAngleY + 0x400);
    MulMatrix2(&yRot, &xRot);
    MulMatrix2(RENDER_VIEW_MATRIX_GTE, &xRot);
    scale[2] = 0x1000;
    scale[0] = 0x1000;
    scale[1] = 0x2000;
    ScaleMatrix(&yRot, scale);
    MulMatrix2(&yRot, &xRot);
    SetGteLightMatrix(&xRot);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, position, &xRot);
    RENDER_ENV_MODE4 = 0;
    SubmitControllerModel(1);
    if (variant != 0) {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, position, &xRot);
        RENDER_ENV_MODE4 = 0;
        model = 1;
        if (g_ModelBankCount >= 4) {
            model = 3;
        }
        SubmitControllerModel(model);
    }

    {
        s32 angle = g_ControllerSceneAngleX - 0x40;

        BuildRotMatrixX(&xRot, angle - steer);
    }
    BuildRotMatrixY(&yRot, g_ControllerSceneAngleY + 0x400);
    MulMatrix2(&yRot, &xRot);
    MulMatrix2(RENDER_VIEW_MATRIX_GTE, &xRot);
    scale[2] = 0x1000;
    scale[0] = 0x1000;
    scale[1] = 0x2000;
    ScaleMatrix(&yRot, scale);
    MulMatrix2(&yRot, &xRot);
    SetGteLightMatrix(&xRot);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, position, &xRot);
    RENDER_ENV_MODE4 = 0;
    model = 1;
    if (g_ModelBankCount >= 3) {
        model = 2;
    }
    SubmitControllerModel(model);
    if (variant != 0) {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, position, &xRot);
        RENDER_ENV_MODE4 = 0;
        model = 1;
        if (g_ModelBankCount >= 5) {
            model = 4;
        }
        SubmitControllerModel(model);
    }
}

/* Wide-parameter view of the packet builders; see GameQueueSprite.c. */
/* The 16x32 left arrow, plus - while `pulse` is set - a tile over it whose
 * green channel breathes with rsin of the shared arrow angle. */
u8 *DrawLeftArrow(void *ot, u8 *prim, s32 x, s32 y, s32 pulse) {
    prim = GameQueueSprite(ot, prim, x, y, 0x10, 0x20, 0x48, 0xB8, 0x7F82);
    prim = QueueDrawModePrim(ot, prim, 0x39);
    if (pulse != 0) {
        u8 glow = rsin(g_SetupArrowPulse % 0x1000) / 64 - 65;

        prim = AddTilePrim(
            ot, prim, x, y, 0x10, 0x20, 0, glow, 0);
    }
    return prim;
}

/* The 16x32 right arrow, plus - while `pulse` is set - a tile over it whose
 * green channel breathes with rsin of the shared arrow angle. */
u8 *DrawRightArrow(void *ot, u8 *prim, s32 x, s32 y, s32 pulse) {
    prim = GameQueueSprite(ot, prim, x, y, 0x10, 0x20, 0x58, 0xB8, 0x7F82);
    prim = QueueDrawModePrim(ot, prim, 0x39);
    if (pulse != 0) {
        u8 glow = rsin(g_SetupArrowPulse % 0x1000) / 64 - 65;

        prim = AddTilePrim(
            ot, prim, x, y, 0x10, 0x20, 0, glow, 0);
    }
    return prim;
}

/*
 * The framed "CONFIG n" panel at (x, y): the caption strip, then the three
 * digit cells (the middle one steps 8 texels per configuration), then the
 * white frame drawn as four nested tiles.
 */
u8 *DrawPadConfigSelector(ot, prim, x, y, selection)
    void *ot;
    u8 *prim;
    s16 x;
    s16 y;
    s16 selection;
{
    prim = GameQueueShadedSprite(
        ot, prim, x + 6, y + 8, 0x30, 0xC, 0x78, 0xC0, 0x7F40);
    prim = QueueDrawModePrim(ot, prim, 0x3A);
    prim = GameQueueSpriteTrans(
        ot, prim, x + 18, y + 32, 8, 0x10, 0x68, 0x28, 0x7F40);
    prim = GameQueueSpriteTrans(
        ot, prim, x + 26, y + 32, 8, 0x10, selection * 8 + 80, 0x18, 0x7F40);
    prim = GameQueueSpriteTrans(
        ot, prim, x + 34, y + 32, 8, 0x10, 0x68, 0x28, 0x7F40);
    prim = QueueDrawModePrim(ot, prim, 0x5B);
    prim = AddTilePrim(
        ot, prim, x + 1, y + 2, 0x3A, 0x14, 0, 0, 0);
    prim = AddTilePrim(
        ot, prim, x + 2, y + 26, 0x38, 0x1A, 0xFF, 0xFF, 0xFF);
    prim = AddTilePrim(
        ot, prim, x + 1, y + 24, 0x3A, 0x1E, 0, 0, 0);
    return AddTilePrim(
        ot, prim, x, y, 0x3C, 0x38, 0xFF, 0xFF, 0xFF);
}
