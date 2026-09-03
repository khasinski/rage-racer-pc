#include "game/asset.h"
#include "game/showroom_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_internal.h"
#include "game/track.h"


static void SwapShowroomCarModel(void) {
    g_CarModelSlot = g_CarModelSlot < 1;
    ActivateShowroomCarModel();
}

static void UpdateMenuViewSpin(void) {
    if ((g_PadHeld & PAD_L1) && g_MenuViewSpin < 64) {
        g_MenuViewSpin++;
    }
    if ((g_PadHeld & PAD_R1) && g_MenuViewSpin >= -63) {
        g_MenuViewSpin--;
    }
}

static void UpdateShowroomSteering(void) {
    if ((g_PadHeld & PAD_R2) && g_PlayerCar.steeringAngle < 6144) {
        g_PlayerCar.steeringAngle += 192;
    }
    if ((g_PadHeld & PAD_L2) && g_PlayerCar.steeringAngle >= -6143) {
        g_PlayerCar.steeringAngle -= 192;
    }
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
    s32 viewHeight;
    int64_t angleDelta;
    s32 currentAngle;
    s32 horizontalAngle;
    s32 offset;

    vec = g_MenuCarPivotOffset;
    SetupMenuViewCamera(0x100, 0);

    currentAngle = g_MenuViewAngle;
    angleDelta = (int64_t)g_MenuViewAngleTarget - currentAngle;
    if (angleDelta != 0) {
        if (angleDelta < 0) {
            if (currentAngle <= 299999) {
                if (g_CarSwapToIndex >= 0) {
                    if (!AssetLoadCompletedSuccessfully()) {
                        return;
                    }
                    SwapShowroomCarModel();
                    g_CarSwapFromIndex = g_CarSwapToIndex;
                    g_CarSwapToIndex = -1;
                } else {
                    g_MenuViewAngle = AdvanceMenuViewAngleValue(
                        currentAngle, g_MenuViewAngleTarget, 24);
                }
            } else {
                g_MenuViewAngle = AdvanceMenuViewAngleValue(
                    currentAngle, g_MenuViewAngleTarget, 24);
            }
        } else {
            if (currentAngle > 900000 && g_CarSwapToIndex >= 0) {
                if (!AssetLoadCompletedSuccessfully()) {
                    return;
                }
                SwapShowroomCarModel();
                g_CarSwapFromIndex = g_CarSwapToIndex;
                g_CarSwapToIndex = -1;
            } else {
                g_MenuViewAngle = AdvanceMenuViewAngleValue(
                    currentAngle, g_MenuViewAngleTarget, 24);
            }
        }
    }

    horizontalAngle = MenuWrapAngle(g_MenuViewAngle, 600000) / 1000;
    carIndex = g_CarSwapFromIndex;
    viewHeight = AdvanceMenuViewOffset();
    showroom->runtime.modelIndex =
        GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant);
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
    MulMatrix2((&g_RenderState.matrix), &mtxA);

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
    SelectModelBank(14);
    /* The render state's ordering-table base. Keep the retail
     * 120-byte (30-entry) showroom-depth bias, but express it through the
     * native pointer-sized slot instead of relying on the absolute-address
     * scalar alias. */
    RENDER_OT_BASE += 30;
    SetGteObjectMatrix((&g_ObjectMatrixWork),
                       AsPositionWords(&showroom->pose.position[0]), &mtxA);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, g_ModelBankCount >= 6 ? 5 : 1);
    RENDER_OT_BASE -= 30;
}

/* The course diorama behind COURSE SELECT and RANKING, with the carousel easing. */
void DrawMenuCourseView(void) {
    ShowroomPlayerCarState *showroom = ShowroomPlayerCar();
    GameRenderObject *renderObject = ShowroomRenderObject();
    Matrix mtxA;
    Matrix mtxB;
    int64_t angleDelta;
    s32 horizontalAngle;
    s32 courseModelIndex;
    s32 viewHeight;

    SetupMenuViewCamera(0x100, 0);

    angleDelta = (int64_t)g_MenuViewAngleTarget - g_MenuViewAngle;
    if (angleDelta != 0) {
        if (angleDelta > 0) {
            if (g_MenuViewAngle > 750000 && g_MenuPendingCourseIndex >= 0) {
                if (g_CourseSwapDelay >= 19) {
                    g_CourseSwapDelay = 0;
                    g_MenuCourseModelIndex = g_MenuPendingCourseIndex;
                    g_MenuPendingCourseIndex = -1;
                } else {
                    g_CourseSwapDelay++;
                }
            } else {
                g_MenuViewAngle = AdvanceMenuViewAngleValue(
                    g_MenuViewAngle, g_MenuViewAngleTarget, 18);
            }
        } else {
            if (g_MenuViewAngle <= 249999 && g_MenuPendingCourseIndex >= 0) {
                if (g_CourseSwapDelay >= 19) {
                    g_CourseSwapDelay = 0;
                    g_MenuCourseModelIndex = g_MenuPendingCourseIndex;
                    g_MenuPendingCourseIndex = -1;
                } else {
                    g_CourseSwapDelay++;
                }
            } else {
                g_MenuViewAngle = AdvanceMenuViewAngleValue(
                    g_MenuViewAngle, g_MenuViewAngleTarget, 18);
            }
        }
    }

    horizontalAngle = MenuWrapAngle(g_MenuViewAngle, 500000) / 1000;
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
    MulMatrix2((&g_RenderState.matrix), &mtxA);
    SelectModelBank(14);
    SetGteObjectMatrix(&g_ObjectMatrixWork,
                       AsPositionWords(&renderObject->x), &mtxA);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState,
                (courseModelIndex & 3) < g_ModelBankCount
                    ? courseModelIndex & 3
                    : 1);
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
    rotationY = g_MenuViewAngle / 1000;
    rotationZ =
        rsin((s32)((u32)g_AnimTimer * 20u & 0xFFCu)) * 72 / 4096;

    BuildRotMatrixY(&mtxB, 0x800 - rotationY);
    BuildRotMatrixZ(&mtxA, rotationZ);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2((&g_RenderState.matrix), &mtxA);
    ScaleMatrix(&mtxA, &vcopy);

    if (g_TeamNameCharModel != 10 &&
        (u32)(g_TeamNameCharModel - 42) >= 2U) {
        SetGteObjectMatrix((&g_ObjectMatrixWork),
                           AsPositionWords(&position.x), &mtxA);
        g_RenderState.envMode4 = 0;
        modelIndex = g_TeamNameCharModel < g_CourseModelCount
                         ? g_TeamNameCharModel
                         : 1;
        SubmitCourseModel(&g_RenderState, modelIndex);
    }
}
