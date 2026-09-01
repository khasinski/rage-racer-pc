#include "game/asset.h"
#include "game/showroom_internal.h"
#include "game/menu.h"
#include "game/render_internal.h"
#include "game/track.h"


/* Flips the double-buffered showroom slot and re-registers it. */
void SwapCarModelSlot(void) {
    g_CarModelSlot = g_CarModelSlot < 1;
    InstallCarModelSlot();
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
    if ((g_PadHeld & PAD_R2) && g_PlayerSteerAngle < 6144) {
        g_PlayerSteerAngle += 192;
    }
    if ((g_PadHeld & PAD_L2) && g_PlayerSteerAngle >= -6143) {
        g_PlayerSteerAngle -= 192;
    }
}

void DrawMenuCarView(void) {
    ShowroomPlayerCarState *showroom = ShowroomPlayerCar();
    GameRenderObject *renderObject = ShowroomRenderObject();
    Matrix mtxA;
    Matrix mtxB;
    Vec4 out;
    Vec4 vec;
    s32 s1, s2, s3;
    s32 x;
    s32 targetAngle;
    s32 offset;

    vec = g_MenuCarPivotOffset;
    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_RenderState.viewX = 0;
    g_RenderState.viewAngleX = 0x100;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;

    SetCameraRotMatrix();
    ScaleMatrix((&g_RenderState.matrix), &g_MenuViewScale);

    if (249999 < g_MenuViewOffsetTarget) {
        if (g_MenuViewOffset < 2500) {
            g_MenuViewOffset = 2500;
        }
    }

    targetAngle = g_MenuViewAngleTarget;
    x = g_MenuViewAngle;
    s3 = targetAngle - x;
    if (s3 != 0) {
        if (s3 < 0) {
            if (x <= 299999) {
                if (g_CarSwapToIndex >= 0) {
                    if (g_AssetLoadState != 0) {
                        return;
                    }
                    SwapCarModelSlot();
                    g_CarSwapFromIndex = g_CarSwapToIndex;
                    g_CarSwapToIndex = -1;
                } else {
                    g_MenuViewAngle = (s3 - 24) / 24 + x;
                }
            } else {
                g_MenuViewAngle = (s3 - 24) / 24 + x;
            }
        } else {
            if (x > 900000 && g_CarSwapToIndex >= 0) {
                if (g_AssetLoadState != 0) {
                    return;
                }
                SwapCarModelSlot();
                g_CarSwapFromIndex = g_CarSwapToIndex;
                g_CarSwapToIndex = -1;
            } else {
                g_MenuViewAngle = (s3 + 24) / 24 + x;
            }
        }
    }

    s3 = ((g_MenuViewAngle + 300000) % 600000 - 300000) / 1000;
    s1 = g_CarSwapFromIndex;
    s2 = g_MenuViewOffsetTarget - g_MenuViewOffset;
    if (s2 != 0) {
        if (s2 > 0) {
            s2 = (250008 - s2) / 8;
        } else {
            s2 = (s2 - 12) / 12;
        }
    }

    g_MenuViewOffset = s2 + g_MenuViewOffset;
    s2 = g_MenuViewOffset / 1000;
    showroom->runtime.modelIndex =
        GetCarAssetIndex(s1, g_CarTable[s1].modelVariant);
    g_PlayerTireCompound = g_CarTable[s1].tireCompound;

    UpdateShowroomSteering();
    g_PlayerTransmission = g_CarTable[s1].transmission;
    g_PlayerCarWheelAngle = (g_PlayerCarWheelAngle + 68) & 0xFFF;

    UpdateMenuViewSpin();
    showroom->pose.rotation.y += g_MenuViewSpin;
    BuildRotMatrixY(&mtxA, showroom->pose.rotation.y);
    vec.z = (s16)(-((s16)g_CarModelAsset->modelOffsetZ / 2));
    ApplyMatrixLV(&mtxA, AsWords(&vec), AsWords(&out));
    BuildRotMatrixY(&mtxB, 0x800 - showroom->pose.rotation.y);
    BuildRotMatrixX(&mtxA, showroom->pose.rotation.x);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2((&g_RenderState.matrix), &mtxA);

    if (g_MenuAltLayout != 0) {
        offset = s3 - 23;
    } else {
        offset = s3 - 52;
    }
    showroom->pose.position[0] = out.x - offset;
    showroom->pose.position[1] = s2 + 30;
    showroom->pose.position[2] = -out.z;
    g_PlayerRenderRotation = showroom->pose.rotation;
    g_PlayerRenderY = showroom->pose.position[1];
    SelectModelBank(g_CarModelSlot);
    DrawPlayerCarModel(renderObject);

    showroom->pose.position[0] =
        (g_MenuAltLayout != 0 ? 23 : 52) - s3;
    showroom->pose.position[1] = s2 + 30;
    showroom->pose.position[2] = 0;
    SelectModelBank(14);
    /* The render state's ordering-table base. Keep the retail
     * 120-byte (30-entry) showroom-depth bias, but express it through the
     * native pointer-sized slot instead of relying on the absolute-address
     * scalar alias. */
    RENDER_OT_BASE_AS(OT_TYPE) += 30;
    SetGteObjectMatrix((&g_ObjectMatrixWork),
                       AsPositionWords(&showroom->pose.position[0]), &mtxA);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, g_ModelBankCount >= 6 ? 5 : 1);
    RENDER_OT_BASE_AS(OT_TYPE) -= 30;
}

/* The course diorama behind COURSE SELECT and RANKING, with the carousel easing. */
void DrawMenuCourseView(void) {
    ShowroomPlayerCarState *showroom = ShowroomPlayerCar();
    GameRenderObject *renderObject = ShowroomRenderObject();
    Matrix mtxA;
    Matrix mtxB;
    s32 s1;
    s32 s0;
    s32 s2;

    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_RenderState.viewX = 0;
    g_RenderState.viewAngleX = 0x100;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;

    SetCameraRotMatrix();
    ScaleMatrix((&g_RenderState.matrix), &g_MenuViewScale);

    if (249999 < g_MenuViewOffsetTarget) {
        if (g_MenuViewOffset < 2500) {
            g_MenuViewOffset = 2500;
        }
    }

    s1 = g_MenuViewAngleTarget - g_MenuViewAngle;
    if (s1 != 0) {
        if (s1 > 0) {
            if (g_MenuViewAngle > 750000 && g_MenuPendingCourseIndex >= 0) {
                if (g_CourseSwapDelay >= 19) {
                    g_CourseSwapDelay = 0;
                    g_MenuCourseModelIndex = g_MenuPendingCourseIndex;
                    g_MenuPendingCourseIndex = -1;
                } else {
                    g_CourseSwapDelay++;
                }
            } else {
                g_MenuViewAngle = (s1 + 18) / 18 + g_MenuViewAngle;
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
                g_MenuViewAngle = (s1 - 18) / 18 + g_MenuViewAngle;
            }
        }
    }

    s1 = ((g_MenuViewAngle + 250000) % 500000 - 250000) / 1000;

    s2 = g_MenuCourseModelIndex;

    s0 = g_MenuViewOffsetTarget - g_MenuViewOffset;
    if (s0 != 0) {
        if (s0 > 0) {
            s0 = (250008 - s0) / 8;
        } else {
            s0 = (s0 - 12) / 12;
        }
    }

    showroom->courseViewX = 23 - s1;
    g_MenuViewOffset = s0 + g_MenuViewOffset;
    showroom->runtime.z = -20;
    s0 = g_MenuViewOffset / 1000;
    showroom->runtime.y = s0 + 15;

    UpdateMenuViewSpin();
    showroom->runtime.bodyYaw += g_MenuViewSpin;
    BuildRotMatrixY(&mtxB, 0x800 - showroom->runtime.bodyYaw);
    BuildRotMatrixX(&mtxA, showroom->runtime.bodyPitch);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2((&g_RenderState.matrix), &mtxA);
    SelectModelBank(14);
    SetGteObjectMatrix(&g_ObjectMatrixWork,
                       AsPositionWords(&renderObject->x), &mtxA);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState,
                (s2 & 3) < g_ModelBankCount ? s2 & 3 : 1);
}

typedef struct MenuModelTransform {
    s32 positionX;
    s32 positionY;
    s32 positionZ;
    s32 reserved0C;
    s32 rotationX;
    s32 rotationY;
    s32 rotationZ;
} MenuModelTransform;


/* The 3D character model under the TEAM NAME grid cursor; skips the BS and ED cells. */
void DrawTeamNameCharModel(void) {
    Matrix mtxA;
    Matrix mtxB;
    MenuModelTransform transform;
    Vec4 vcopy;
    s32 s1;
    s32 s0;
    s32 s2;
    s32 nextAngle;
    s32 modelIndex;

    vcopy = g_TeamNameCharScale;

    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_RenderState.viewX = 0;
    g_RenderState.viewAngleX = 0;
    g_RenderState.viewAngleY = -104;
    g_RenderState.viewAngleZ = 0;

    SetCameraRotMatrix();
    ScaleMatrix((&g_RenderState.matrix), &g_MenuViewScale);

    if (249999 < g_MenuViewOffsetTarget) {
        if (g_MenuViewOffset < 2500) {
            g_MenuViewOffset = 2500;
        }
    }

    s1 = g_MenuViewAngleTarget - g_MenuViewAngle;
    if (s1 != 0) {
        if (s1 > 0) {
            s1 = (s1 + 16) / 16;
        } else {
            s1 = (s1 - 16) / 16;
        }
    }

    nextAngle = g_MenuViewAngle + s1;
    g_MenuViewAngle = nextAngle;
    if (nextAngle <= 3071999 && GameMenuCursorAnim >= 0) {
        g_MenuViewAngle = nextAngle - 2048000;
        g_TeamNameCharModel = GameMenuCursorAnim;
        GameMenuCursorAnim = -1;
    }

    s1 = g_MenuViewAngle / 1000;

    s0 = g_MenuViewOffsetTarget - g_MenuViewOffset;
    if (s0 != 0) {
        if (s0 > 0) {
            s0 = (250008 - s0) / 8;
        } else {
            s0 = (s0 - 12) / 12;
        }
    }

    g_MenuViewOffset = s0 + g_MenuViewOffset;
    s0 = g_MenuViewOffset / 1000;

    s2 = 40;
    if (g_MenuAltLayout != 0) {
        s2 = 64;
    }

    transform.positionX = 0;
    transform.positionY =
        (s0 - s2) + rsin((g_AnimTimer * 32) & 0xFE0) * 12 / 4096;
    transform.positionZ = 0;
    transform.rotationX = 0;
    transform.rotationY = s1;
    transform.rotationZ =
        rsin((g_AnimTimer * 20) & 0xFFC) * 72 / 4096;

    BuildRotMatrixY(&mtxB, 0x800 - transform.rotationY);
    BuildRotMatrixZ(&mtxA, transform.rotationZ);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2((&g_RenderState.matrix), &mtxA);
    ScaleMatrix(&mtxA, &vcopy);

    if (g_TeamNameCharModel != 10 &&
        (u32)(g_TeamNameCharModel - 42) >= 2U) {
        SetGteObjectMatrix((&g_ObjectMatrixWork),
                           AsPositionWords(&transform.positionX), &mtxA);
        g_RenderState.envMode4 = 0;
        modelIndex = g_TeamNameCharModel < g_CourseModelCount
                         ? g_TeamNameCharModel
                         : 1;
        SubmitCourseModel(&g_RenderState, modelIndex);
    }
}
