#include "game/asset.h"
#include "game/course_index.h"
#include "game/showroom_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_internal.h"
#include "game/track.h"

enum {
    MENU_VIEW_FIXED_SCALE = 1000,
    CAR_VIEW_ANGLE_PERIOD = 600000,
    COURSE_VIEW_ANGLE_PERIOD = 500000,
    SHOWROOM_FLOOR_MODEL = 5,
    SHOWROOM_MODEL_BANK = 14,
    SHOWROOM_OT_DEPTH_BIAS = 30,
};

static s32 SwapShowroomCarModel(void) {
    return ActivateShowroomCarModel(g_CarModelSlot < 1);
}

static void UpdateMenuViewSpin(void) {
    g_MenuViewSpin = UpdatedMenuViewSpin(g_MenuViewSpin, g_PadHeld);
}

static void UpdateShowroomSteering(void) {
    g_PlayerCar.steeringAngle =
        UpdatedShowroomSteering(g_PlayerCar.steeringAngle, g_PadHeld);
}

static void SetupMenuViewCamera(s32 pitch, s32 yaw) {
    g_RenderState.viewX = 0;
    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_RenderState.viewAngleX = pitch;
    g_RenderState.viewAngleY = yaw;
    g_RenderState.viewAngleZ = 0;
    SetCameraRotMatrix();
    ScaleMatrix(&g_RenderState.matrix, &g_MenuViewScale);

    if (g_MenuViewOffsetTarget > 249999 && g_MenuViewOffset < 2500) {
        g_MenuViewOffset = 2500;
    }
}

static s32 AdvanceMenuViewOffset(void) {
    g_MenuViewOffset = AdvanceMenuViewOffsetValue(
        g_MenuViewOffset, g_MenuViewOffsetTarget);
    return g_MenuViewOffset / 1000;
}

void DrawMenuCarView(void) {
    ShowroomPlayerCarState *showroom = ShowroomPlayerCar();
    GameRenderObject *renderObject = ShowroomRenderObject();
    Matrix mtxA;
    Matrix mtxB;
    Vec4 out;
    Vec4 vec;
    s32 carIndex;
    s32 modelIndex;
    s32 viewHeight;
    s32 currentAngle;
    s32 horizontalAngle;
    s32 offset;

    vec = g_MenuCarPivotOffset;
    SetupMenuViewCamera(0x100, 0);

    currentAngle = g_MenuViewAngle;
    if (ShowroomCarAtSwapPoint(currentAngle, g_MenuViewAngleTarget,
                               g_CarSwapToIndex)) {
        if (!AssetLoadCompletedSuccessfully()) {
            return;
        }
        if (!SwapShowroomCarModel()) {
            g_CarSwapToIndex = -1;
            return;
        }
        g_CarSwapFromIndex = g_CarSwapToIndex;
        g_CarSwapToIndex = -1;
    } else if (currentAngle != g_MenuViewAngleTarget) {
        g_MenuViewAngle = AdvanceMenuViewAngleValue(
            currentAngle, g_MenuViewAngleTarget, 24);
    }

    horizontalAngle =
        MenuWrapAngle(g_MenuViewAngle, CAR_VIEW_ANGLE_PERIOD) /
        MENU_VIEW_FIXED_SCALE;
    carIndex = g_CarSwapFromIndex;
    viewHeight = AdvanceMenuViewOffset();
    if ((u32)carIndex >= GAME_CAR_COUNT || g_CarTable == NULL ||
        g_CarModelAsset == NULL) {
        return;
    }
    showroom->runtime.modelIndex =
        GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant);
    if (showroom->runtime.modelIndex < 0) {
        return;
    }
    g_PlayerCar.showroomTireCompound = g_CarTable[carIndex].tireCompound;

    UpdateShowroomSteering();
    g_PlayerCar.drive.manual = g_CarTable[carIndex].transmission;
    g_PlayerCar.wheelRotation =
        ((u32)g_PlayerCar.wheelRotation + 68u) & 0xFFFu;

    UpdateMenuViewSpin();
    showroom->pose.rotation.y =
        (s32)((u32)showroom->pose.rotation.y + (u32)g_MenuViewSpin);
    BuildRotMatrixY(&mtxA, showroom->pose.rotation.y);
    vec.z = (s16)(-((s16)g_CarModelAsset->modelOffsetZ / 2));
    ApplyMatrixLV(&mtxA, AsWords(&vec), AsWords(&out));
    BuildRotMatrixY(&mtxB, 0x800 - showroom->pose.rotation.y);
    BuildRotMatrixX(&mtxA, showroom->pose.rotation.x);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(&g_RenderState.matrix, &mtxA);

    if (g_MenuAltLayout != 0) {
        offset = horizontalAngle - 23;
    } else {
        offset = horizontalAngle - 52;
    }
    showroom->pose.position[0] = out.x - offset;
    showroom->pose.position[1] = viewHeight + 30;
    showroom->pose.position[2] = -out.z;
    g_PlayerCar.modelRotation = showroom->pose.rotation;
    g_PlayerCar.modelY = showroom->pose.position[1];
    SelectModelBank(g_CarModelSlot);
    DrawPlayerCarModel(renderObject);

    showroom->pose.position[0] =
        (g_MenuAltLayout != 0 ? 23 : 52) - horizontalAngle;
    showroom->pose.position[1] = viewHeight + 30;
    showroom->pose.position[2] = 0;
    SelectModelBank(SHOWROOM_MODEL_BANK);
    /* The render state's ordering-table base. Keep the retail
     * 120-byte (30-entry) showroom-depth bias, but express it through the
     * native pointer-sized slot instead of relying on the absolute-address
     * scalar alias. */
    RENDER_OT_BASE += SHOWROOM_OT_DEPTH_BIAS;
    SetGteObjectMatrix(AsPositionWords(&showroom->pose.position[0]), &mtxA);
    g_RenderState.envMode4 = 0;
    modelIndex = MenuModelIndexOrFallback(SHOWROOM_FLOOR_MODEL,
                                          g_ModelBankCount);
    if (modelIndex >= 0) {
        SubmitModel(&g_RenderState, modelIndex);
    }
    RENDER_OT_BASE -= SHOWROOM_OT_DEPTH_BIAS;
}

/* The course diorama behind COURSE SELECT and RANKING, with the carousel easing. */
void DrawMenuCourseView(void) {
    ShowroomPlayerCarState *showroom = ShowroomPlayerCar();
    GameRenderObject *renderObject = ShowroomRenderObject();
    Matrix mtxA;
    Matrix mtxB;
    s32 horizontalAngle;
    s32 courseModelIndex;
    s32 viewHeight;

    SetupMenuViewCamera(0x100, 0);

    if (CourseCarouselAtSwapPoint(g_MenuViewAngle, g_MenuViewAngleTarget,
                                  g_MenuPendingCourseIndex)) {
        g_CourseSwapDelay = NormalizeCourseSwapDelay(g_CourseSwapDelay);
        if (g_CourseSwapDelay >= 19) {
            g_CourseSwapDelay = 0;
            g_MenuCourseModelIndex = g_MenuPendingCourseIndex;
            g_MenuPendingCourseIndex = -1;
        } else {
            g_CourseSwapDelay++;
        }
    } else if (g_MenuViewAngle != g_MenuViewAngleTarget) {
        g_MenuViewAngle = AdvanceMenuViewAngleValue(
            g_MenuViewAngle, g_MenuViewAngleTarget, 18);
    }

    horizontalAngle =
        MenuWrapAngle(g_MenuViewAngle, COURSE_VIEW_ANGLE_PERIOD) /
        MENU_VIEW_FIXED_SCALE;
    courseModelIndex = g_MenuCourseModelIndex;
    viewHeight = AdvanceMenuViewOffset();

    showroom->courseViewX = 23 - horizontalAngle;
    showroom->runtime.z = -20;
    showroom->runtime.y = viewHeight + 15;

    UpdateMenuViewSpin();
    showroom->runtime.bodyYaw =
        (s32)((u32)showroom->runtime.bodyYaw + (u32)g_MenuViewSpin);
    BuildRotMatrixY(&mtxB, 0x800 - showroom->runtime.bodyYaw);
    BuildRotMatrixX(&mtxA, showroom->runtime.bodyPitch);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(&g_RenderState.matrix, &mtxA);
    SelectModelBank(SHOWROOM_MODEL_BANK);
    SetGteObjectMatrix(AsPositionWords(&renderObject->x), &mtxA);
    g_RenderState.envMode4 = 0;
    courseModelIndex = MenuModelIndexOrFallback(
        CourseSlot(courseModelIndex), g_ModelBankCount);
    if (courseModelIndex >= 0) {
        SubmitModel(&g_RenderState, courseModelIndex);
    }
}

/* The 3D character model under the TEAM NAME grid cursor; skips the BS and ED cells. */
void DrawTeamNameCharModel(void) {
    Matrix mtxA;
    Matrix mtxB;
    Vec4 position;
    Vec4 vcopy;
    s32 viewHeight;
    s32 baseHeight;
    s32 nextAngle;
    s32 modelIndex;
    s32 rotationY;
    s32 rotationZ;

    vcopy = g_TeamNameCharScale;

    SetupMenuViewCamera(0, -104);

    nextAngle = AdvanceMenuViewAngleValue(
        g_MenuViewAngle, g_MenuViewAngleTarget, 16);
    g_MenuViewAngle = nextAngle;
    if (nextAngle <= 3071999 && GameMenuCursorAnim >= 0) {
        g_MenuViewAngle = (s32)((u32)nextAngle - 2048000u);
        g_TeamNameCharModel = GameMenuCursorAnim;
        GameMenuCursorAnim = -1;
    }

    viewHeight = AdvanceMenuViewOffset();
    baseHeight = 40;
    if (g_MenuAltLayout != 0) {
        baseHeight = 64;
    }

    position.x = 0;
    position.y =
        (viewHeight - baseHeight) +
        rsin((s32)((u32)g_AnimTimer * 32u & 0xFE0u)) * 12 / 4096;
    position.z = 0;
    position.w = 0;
    rotationY = g_MenuViewAngle / MENU_VIEW_FIXED_SCALE;
    rotationZ =
        rsin((s32)((u32)g_AnimTimer * 20u & 0xFFCu)) * 72 / 4096;

    BuildRotMatrixY(&mtxB, 0x800 - rotationY);
    BuildRotMatrixZ(&mtxA, rotationZ);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2((&g_RenderState.matrix), &mtxA);
    ScaleMatrix(&mtxA, &vcopy);

    modelIndex = TeamNameCharacterModelIndex(g_TeamNameCharModel,
                                             g_CourseModelCount);
    if (modelIndex >= 0) {
        SetGteObjectMatrix(AsPositionWords(&position.x), &mtxA);
        g_RenderState.envMode4 = 0;
        SubmitCourseModel(&g_RenderState, modelIndex);
    }
}
