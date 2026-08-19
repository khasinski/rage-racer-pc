#include "common.h"
#include "game/game_input.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/showroom_internal.h"
#include "game/asset_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/scratchpad_legacy.h"
#include "game/state.h"
#include "game/track.h"
#include "game/vector.h"
#include "psyq/gte.h"
#include "game/menu_context.h"


/* Flips the double-buffered showroom slot and re-registers it. */
void SwapCarModelSlot(void) {
    g_CarModelSlot = g_CarModelSlot < 1;
    InstallCarModelSlot();
}


void DrawCarSlotHighlight(s32 slot) {
    u8 **scratch = &RENDER_PRIM_CURSOR_AS(u8);
    u8 *value = *scratch;

    *scratch = GameQueueTileTrans(
        GamePrimaryOrderingTable(0), value, 0x24, (slot * 16) + 0x24,
        0x50, 0x10, 0, 0, 0xFF);
}

void DrawMenuCarView(void) {
    Matrix mtxA;
    Matrix mtxB;
    Vec4 out;
    Vec4 vec;
    s32 carIndex;
    s32 viewOffsetStep;
    s32 displayedAngle;
    s32 x;
    s32 targetAngle;
    s32 offset;
    s32 outX;
    s32 altLayout;
    s32 result;
    s32 outZ;
    s32 qValue;
    s32 modelSlot;
    s32 *p;
    s32 *q;

    vec = g_MenuCarPivotOffset;
    RENDER_VIEW_Y = -64;
    RENDER_VIEW_Z = -256;
    RENDER_VIEW_X = 0;
    RENDER_VIEW_ANGLE_X = 0x100;
    RENDER_VIEW_ANGLE_Y = 0;
    RENDER_VIEW_ANGLE_Z = 0;

    SetCameraRotMatrix();
    ScaleMatrix(RENDER_VIEW_MATRIX_GTE, &g_MenuViewScale);

    if (249999 < g_MenuViewOffsetTarget) {
        if (g_MenuViewOffset < 2500) {
            g_MenuViewOffset = 2500;
        }
    }

    targetAngle = g_MenuViewAngleTarget;
    x = g_MenuViewAngle;
    displayedAngle = targetAngle - x;
    if (displayedAngle != 0) {
        if (displayedAngle < 0) {
            if (x <= 299999) {
                if (g_CarSwapToIndex >= 0) {
                    if (g_AssetLoadState != 0) {
                        return;
                    }
                    SwapCarModelSlot();
                    g_CarSwapFromIndex = g_CarSwapToIndex;
                    g_CarSwapToIndex = -1;
                } else {
                    g_MenuViewAngle = (displayedAngle - 24) / 24 + x;
                }
            } else {
                g_MenuViewAngle = (displayedAngle - 24) / 24 + x;
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
                g_MenuViewAngle = (displayedAngle + 24) / 24 + x;
            }
        }
    }

    displayedAngle = ((g_MenuViewAngle + 300000) % 600000 - 300000) / 1000;
    carIndex = g_CarSwapFromIndex;
    viewOffsetStep = g_MenuViewOffsetTarget - g_MenuViewOffset;
    if (viewOffsetStep != 0) {
        if (viewOffsetStep > 0) {
            viewOffsetStep = (250008 - viewOffsetStep) / 8;
        } else {
            viewOffsetStep = (viewOffsetStep - 12) / 12;
        }
    }

    g_MenuViewOffset = viewOffsetStep + g_MenuViewOffset;
    viewOffsetStep = g_MenuViewOffset / 1000;
    g_PlayerCar.runtime.modelIndex = GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant);
    g_PlayerTireCompound = g_CarTable[carIndex].tireCompound;

    if (g_GameInput.held & 2) {
        if (g_PlayerSteerAngle < 6144) {
            g_PlayerSteerAngle = g_PlayerSteerAngle + 192;
        }
    }
    if (g_GameInput.held % 2) {
        s32 *w = &g_PlayerSteerAngle;
        if (*w >= -6143) {
            *w = *w - 192;
        }
    }

    g_PlayerTransmission = g_CarTable[carIndex].transmission;
    g_PlayerCarWheelAngle = (g_PlayerCarWheelAngle + 68) & 0xFFF;

    if (g_GameInput.held & 4) {
        if (g_MenuViewSpin < 64) {
            g_MenuViewSpin = g_MenuViewSpin + 1;
        }
    }
    if (g_GameInput.held & 8) {
        if (g_MenuViewSpin >= -63) {
            g_MenuViewSpin = g_MenuViewSpin - 1;
        }
    }

    p = &g_PlayerCar.pose.rotation.y;
    *p = *p + g_MenuViewSpin;
    BuildRotMatrixY(&mtxA, *p);
    vec.z = (s16)(-((s16)g_CarModelAsset->modelOffsetZ / 2));
    ApplyMatrixLV(&mtxA, &vec, &out);
    BuildRotMatrixY(&mtxB, 0x800 - *p);
    BuildRotMatrixX(&mtxA, g_PlayerCar.pose.rotation.x);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(RENDER_VIEW_MATRIX_GTE, &mtxA);

    altLayout = g_MenuAltLayout;
    outX = out.x;
    asm volatile("" : "=r"(altLayout), "=r"(outX) : "0"(altLayout), "1"(outX));
    p = &g_PlayerCar.pose.position[0];
    if (altLayout != 0) {
        offset = displayedAngle - 23;
    } else {
        offset = displayedAngle - 52;
    }
    result = outX - offset;
    modelSlot = g_CarModelSlot;
    asm volatile("" : "=r"(result), "=r"(modelSlot) : "0"(result), "1"(modelSlot));
    q = &g_PlayerCar.pose.position[1];
    *p = result;
    outZ = out.z;
    qValue = viewOffsetStep + 30;
    *q = qValue;
    g_PlayerCar.pose.position[2] = -outZ;
    g_PlayerRenderRotation = g_PlayerCar.pose.rotation;
    g_PlayerRenderY = *q;
    SelectModelBank(modelSlot);
    q--;
    DrawPlayerCarModel((GameRenderObject *)q);

    *q = (g_MenuAltLayout != 0 ? 23 : 52) - displayedAngle;
    q = &g_PlayerCar.pose.position[1];
    *q = viewOffsetStep + 30;
    g_PlayerCar.pose.position[2] = 0;
    SelectModelBank(14);
    /* 0x1f800004 is the scratchpad OT-base slot on PS1.  Keep the retail
     * 120-byte (30-entry) showroom-depth bias, but express it through the
     * native pointer-sized slot instead of relying on the absolute-address
     * scalar alias. */
    RENDER_OT_BASE_AS(OT_TYPE) += 30;
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &g_PlayerCar.pose.position[0], &mtxA);
    RENDER_ENV_MODE4 = 0;
    {
        s32 modelIndex = 1;
        if (g_ModelBankCount >= 6) {
            modelIndex = 5;
        }
        SubmitModel(RENDER_WORKSPACE, modelIndex);
    }
    RENDER_OT_BASE_AS(OT_TYPE) -= 30;
}

/* The course diorama behind COURSE SELECT and RANKING, with the carousel easing. */
void DrawMenuCourseView(void) {
    Matrix mtxA;
    Matrix mtxB;
    s32 displayedAngle;
    s32 viewOffsetStep;
    s32 courseModelIndex;
    s32 *p;

    RENDER_VIEW_Y = -64;
    RENDER_VIEW_Z = -256;
    RENDER_VIEW_X = 0;
    RENDER_VIEW_ANGLE_X = 0x100;
    RENDER_VIEW_ANGLE_Y = 0;
    RENDER_VIEW_ANGLE_Z = 0;

    SetCameraRotMatrix();
    ScaleMatrix(RENDER_VIEW_MATRIX_GTE, &g_MenuViewScale);

    if (249999 < g_MenuViewOffsetTarget) {
        if (g_MenuViewOffset < 2500) {
            g_MenuViewOffset = 2500;
        }
    }

    displayedAngle = g_MenuViewAngleTarget - g_MenuViewAngle;
    if (displayedAngle != 0) {
        if (displayedAngle > 0) {
            if (g_MenuViewAngle > 750000 && g_MenuPendingCourseIndex >= 0) {
                if (g_CourseSwapDelay >= 19) {
                    g_CourseSwapDelay = 0;
                    g_MenuCourseModelIndex = g_MenuPendingCourseIndex;
                    g_MenuPendingCourseIndex = -1;
                } else {
                    g_CourseSwapDelay = g_CourseSwapDelay + 1;
                }
            } else {
                g_MenuViewAngle = (displayedAngle + 18) / 18 + g_MenuViewAngle;
            }
        } else {
            if (g_MenuViewAngle <= 249999 && g_MenuPendingCourseIndex >= 0) {
                if (g_CourseSwapDelay >= 19) {
                    g_CourseSwapDelay = 0;
                    g_MenuCourseModelIndex = g_MenuPendingCourseIndex;
                    g_MenuPendingCourseIndex = -1;
                } else {
                    g_CourseSwapDelay = g_CourseSwapDelay + 1;
                }
            } else {
                g_MenuViewAngle = (displayedAngle - 18) / 18 + g_MenuViewAngle;
            }
        }
    }

    displayedAngle = ((g_MenuViewAngle + 250000) % 500000 - 250000) / 1000;

    courseModelIndex = g_MenuCourseModelIndex;

    viewOffsetStep = g_MenuViewOffsetTarget - g_MenuViewOffset;
    if (viewOffsetStep != 0) {
        if (viewOffsetStep > 0) {
            viewOffsetStep = (250008 - viewOffsetStep) / 8;
        } else {
            viewOffsetStep = (viewOffsetStep - 12) / 12;
        }
    }

    g_PlayerCar.courseViewX = 23 - displayedAngle;
    g_MenuViewOffset = viewOffsetStep + g_MenuViewOffset;
    g_PlayerCar.runtime.z = -20;
    viewOffsetStep = g_MenuViewOffset / 1000;
    g_PlayerCar.runtime.y = viewOffsetStep + 15;

    if (g_GameInput.held & 4) {
        if (g_MenuViewSpin < 64) {
            g_MenuViewSpin = g_MenuViewSpin + 1;
        }
    }
    if (g_GameInput.held & 8) {
        if (g_MenuViewSpin >= -63) {
            g_MenuViewSpin = g_MenuViewSpin - 1;
        }
    }

    p = &g_PlayerCar.runtime.bodyYaw;
    *p = *p + g_MenuViewSpin;
    BuildRotMatrixY(&mtxB, 0x800 - *p);
    BuildRotMatrixX(&mtxA, g_PlayerCar.runtime.bodyPitch);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(RENDER_VIEW_MATRIX_GTE, &mtxA);
    SelectModelBank(14);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, p - 9, &mtxA);
    RENDER_ENV_MODE4 = 0;
    {
        s32 modelIndex = 1;
        if ((courseModelIndex & 3) < g_ModelBankCount) {
            modelIndex = courseModelIndex & 3;
        }
        SubmitModel(RENDER_WORKSPACE, modelIndex);
    }
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
    s32 viewAngle;
    s32 viewOffsetStep;
    s32 baseHeight;

    vcopy = g_TeamNameCharScale;

    RENDER_VIEW_Y = -64;
    RENDER_VIEW_Z = -256;
    RENDER_VIEW_X = 0;
    RENDER_VIEW_ANGLE_X = 0;
    RENDER_VIEW_ANGLE_Y = -104;
    RENDER_VIEW_ANGLE_Z = 0;

    SetCameraRotMatrix();
    ScaleMatrix(RENDER_VIEW_MATRIX_GTE, &g_MenuViewScale);

    if (249999 < g_MenuViewOffsetTarget) {
        if (g_MenuViewOffset < 2500) {
            g_MenuViewOffset = 2500;
        }
    }

    viewAngle = g_MenuViewAngleTarget - g_MenuViewAngle;
    if (viewAngle != 0) {
        if (viewAngle > 0) {
            viewAngle = (viewAngle + 16) / 16;
        } else {
            viewAngle = (viewAngle - 16) / 16;
        }
    }

    {
        s32 nextAngle = g_MenuViewAngle + viewAngle;
        g_MenuViewAngle = nextAngle;
        if (nextAngle <= 3071999) {
            s32 pendingModel = GameMenuCursorAnim;
            if (pendingModel >= 0) {
                g_MenuViewAngle = nextAngle - 2048000;
                g_TeamNameCharModel = pendingModel;
                GameMenuCursorAnim = -1;
            }
        }
    }

    viewAngle = g_MenuViewAngle / 1000;

    viewOffsetStep = g_MenuViewOffsetTarget - g_MenuViewOffset;
    if (viewOffsetStep != 0) {
        if (viewOffsetStep > 0) {
            viewOffsetStep = (250008 - viewOffsetStep) / 8;
        } else {
            viewOffsetStep = (viewOffsetStep - 12) / 12;
        }
    }

    g_MenuViewOffset = viewOffsetStep + g_MenuViewOffset;
    viewOffsetStep = g_MenuViewOffset / 1000;

    baseHeight = 40;
    if (g_MenuAltLayout != 0) {
        baseHeight = 64;
    }

    transform.positionX = 0;
    transform.positionY =
        (viewOffsetStep - baseHeight) +
        rsin((g_AnimTimer * 32) & 0xFE0) * 12 / 4096;
    transform.positionZ = 0;
    transform.rotationX = 0;
    transform.rotationY = viewAngle;
    transform.rotationZ =
        rsin((g_AnimTimer * 20) & 0xFFC) * 72 / 4096;

    BuildRotMatrixY(&mtxB, 0x800 - transform.rotationY);
    BuildRotMatrixZ(&mtxA, transform.rotationZ);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(RENDER_VIEW_MATRIX_GTE, &mtxA);
    ScaleMatrix(&mtxA, &vcopy);

    if (g_TeamNameCharModel != 10 &&
        (u32)(g_TeamNameCharModel - 42) >= 2U) {
        s32 modelIndex;
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &transform, &mtxA);
        RENDER_ENV_MODE4 = 0;
        modelIndex = 1;
        if (g_TeamNameCharModel < g_CourseModelCount) {
            modelIndex = g_TeamNameCharModel;
        }
        SubmitCourseModel(RENDER_WORKSPACE, modelIndex);
    }
}

void DrawCarSlotLabel(s32 x, s32 y, s32 label) { DrawText8x8(x, y, g_CarManufacturerNames[label]); }
