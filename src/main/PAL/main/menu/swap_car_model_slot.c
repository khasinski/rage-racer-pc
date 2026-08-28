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

void DrawMenuCarView(void) {
    Matrix mtxA;
    Matrix mtxB;
    Vec4 out;
    Vec4 vec;
    s32 s1, s2, s3;
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
    SCRATCH_VIEW_Y = -64;
    SCRATCH_VIEW_Z = -256;
    SCRATCH_VIEW_X = 0;
    SCRATCH_VIEW_ANGLE_X = 0x100;
    SCRATCH_VIEW_ANGLE_Y = 0;
    SCRATCH_VIEW_ANGLE_Z = 0;

    SetCameraRotMatrix();
    ScaleMatrix(SCRATCH_VIEW_MATRIX_GTE, &g_MenuViewScale);

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
    ShowroomPlayerCar()->runtime.modelIndex = GetCarAssetIndex(s1, g_CarTable[s1].modelVariant);
    g_PlayerTireCompound = g_CarTable[s1].tireCompound;

    if (g_PadHeld & 2) {
        if (g_PlayerSteerAngle < 6144) {
            g_PlayerSteerAngle = g_PlayerSteerAngle + 192;
        }
    }
    if (g_PadHeld % 2) {
        s32 *w = &g_PlayerSteerAngle;
        if (*w >= -6143) {
            *w = *w - 192;
        }
    }

    g_PlayerTransmission = g_CarTable[s1].transmission;
    g_PlayerCarWheelAngle = (g_PlayerCarWheelAngle + 68) & 0xFFF;

    if (g_PadHeld & 4) {
        if (g_MenuViewSpin < 64) {
            g_MenuViewSpin++;
        }
    }
    if (g_PadHeld & 8) {
        if (g_MenuViewSpin >= -63) {
            g_MenuViewSpin--;
        }
    }

    p = &ShowroomPlayerCar()->pose.rotation.y;
    *p = *p + g_MenuViewSpin;
    BuildRotMatrixY(&mtxA, *p);
    vec.z = (s16)(-((s16)g_CarModelAsset->modelOffsetZ / 2));
    ApplyMatrixLV(&mtxA, &vec, &out);
    BuildRotMatrixY(&mtxB, 0x800 - *p);
    BuildRotMatrixX(&mtxA, ShowroomPlayerCar()->pose.rotation.x);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtxA);

    altLayout = g_MenuAltLayout;
    outX = out.x;
    
    p = &ShowroomPlayerCar()->pose.position[0];
    if (altLayout != 0) {
        offset = s3 - 23;
    } else {
        offset = s3 - 52;
    }
    result = outX - offset;
    modelSlot = g_CarModelSlot;
    
    q = &ShowroomPlayerCar()->pose.position[1];
    *p = result;
    outZ = out.z;
    qValue = s2 + 30;
    *q = qValue;
    ShowroomPlayerCar()->pose.position[2] = -outZ;
    g_PlayerRenderRotation = ShowroomPlayerCar()->pose.rotation;
    g_PlayerRenderY = *q;
    SelectModelBank(modelSlot);
    q--;
    DrawPlayerCarModel((GameRenderObject *)q);

    *q = (g_MenuAltLayout != 0 ? 23 : 52) - s3;
    q = &ShowroomPlayerCar()->pose.position[1];
    *q = s2 + 30;
    ShowroomPlayerCar()->pose.position[2] = 0;
    SelectModelBank(14);
    /* 0x1f800004 is the scratchpad OT-base slot on PS1.  Keep the retail
     * 120-byte (30-entry) showroom-depth bias, but express it through the
     * native pointer-sized slot instead of relying on the absolute-address
     * scalar alias. */
    SCRATCH_OT_BASE_AS(OT_TYPE) += 30;
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &ShowroomPlayerCar()->pose.position[0], &mtxA);
    SCRATCH_ENV_MODE4 = 0;
    {
        s32 a1 = 1;
        if (g_ModelBankCount >= 6) {
            a1 = 5;
        }
        SubmitModel(SCRATCHPAD, a1);
    }
    SCRATCH_OT_BASE_AS(OT_TYPE) -= 30;
}

/* The course diorama behind COURSE SELECT and RANKING, with the carousel easing. */
void DrawMenuCourseView(void) {
    Matrix mtxA;
    Matrix mtxB;
    s32 s1;
    s32 s0;
    s32 s2;
    s32 *p;

    SCRATCH_VIEW_Y = -64;
    SCRATCH_VIEW_Z = -256;
    SCRATCH_VIEW_X = 0;
    SCRATCH_VIEW_ANGLE_X = 0x100;
    SCRATCH_VIEW_ANGLE_Y = 0;
    SCRATCH_VIEW_ANGLE_Z = 0;

    SetCameraRotMatrix();
    ScaleMatrix(SCRATCH_VIEW_MATRIX_GTE, &g_MenuViewScale);

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

    ShowroomPlayerCar()->courseViewX = 23 - s1;
    g_MenuViewOffset = s0 + g_MenuViewOffset;
    ShowroomPlayerCar()->runtime.z = -20;
    s0 = g_MenuViewOffset / 1000;
    ShowroomPlayerCar()->runtime.y = s0 + 15;

    if (g_PadHeld & 4) {
        if (g_MenuViewSpin < 64) {
            g_MenuViewSpin++;
        }
    }
    if (g_PadHeld & 8) {
        if (g_MenuViewSpin >= -63) {
            g_MenuViewSpin--;
        }
    }

    p = &ShowroomPlayerCar()->runtime.bodyYaw;
    *p = *p + g_MenuViewSpin;
    BuildRotMatrixY(&mtxB, 0x800 - *p);
    BuildRotMatrixX(&mtxA, ShowroomPlayerCar()->runtime.bodyPitch);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtxA);
    SelectModelBank(14);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, p - 9, &mtxA);
    SCRATCH_ENV_MODE4 = 0;
    {
        s32 a1 = 1;
        if ((s2 & 3) < g_ModelBankCount) {
            a1 = s2 & 3;
        }
        SubmitModel(SCRATCHPAD, a1);
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
    s32 s1;
    s32 s0;
    s32 s2;

    vcopy = g_TeamNameCharScale;

    SCRATCH_VIEW_Y = -64;
    SCRATCH_VIEW_Z = -256;
    SCRATCH_VIEW_X = 0;
    SCRATCH_VIEW_ANGLE_X = 0;
    SCRATCH_VIEW_ANGLE_Y = -104;
    SCRATCH_VIEW_ANGLE_Z = 0;

    SetCameraRotMatrix();
    ScaleMatrix(SCRATCH_VIEW_MATRIX_GTE, &g_MenuViewScale);

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

    {
        s32 t = g_MenuViewAngle + s1;
        g_MenuViewAngle = t;
        if (t <= 3071999) {
            s32 a = GameMenuCursorAnim;
            if (a >= 0) {
                g_MenuViewAngle = t - 2048000;
                g_TeamNameCharModel = a;
                GameMenuCursorAnim = -1;
            }
        }
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
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtxA);
    ScaleMatrix(&mtxA, &vcopy);

    if (g_TeamNameCharModel != 10 &&
        (u32)(g_TeamNameCharModel - 42) >= 2U) {
        s32 a1;
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &transform, &mtxA);
        SCRATCH_ENV_MODE4 = 0;
        a1 = 1;
        if (g_TeamNameCharModel < g_CourseModelCount) {
            a1 = g_TeamNameCharModel;
        }
        SubmitCourseModel(SCRATCHPAD, a1);
    }
}

void DrawCarSlotLabel(s32 x, s32 y, s32 label) { DrawText8x8(x, y, g_CarManufacturerNames[label]); }
