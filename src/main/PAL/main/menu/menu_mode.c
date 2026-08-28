#include "game/car.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/render_internal.h"


void DrawMenuAltPanel(s32 stepA, s32 stepB) {
    s32 step0;
    s32 step1;
    void *scratch;
    s32 value;
    s32 offset;
    s32 x0;
    s32 y0;
    s32 render1;
    s16 y1;
    void *callScratch;
    s32 callX;

    scratch = SCRATCH_OT_BASE;
    step0 = stepA;
    step1 = stepB;

    if (step0 == 0 && step1 == 0) {
        g_MenuAltPanelProgressA = step0;
        g_MenuAltPanelProgressB = step1;
        return;
    }

    if (step0 < 0) {
        value = g_MenuAltPanelProgressA + step0;
        g_MenuAltPanelProgressA = value;
        if (value < 0) {
            g_MenuAltPanelProgressA = 0;
        }
    }

    if (step1 < 0) {
        value = g_MenuAltPanelProgressB + step1;
        g_MenuAltPanelProgressB = value;
        if (value < 0) {
            g_MenuAltPanelProgressB = 0;
        }
    }

    value = g_MenuAltPanelProgressA;
    if (value != 0) {
        offset = (value - 1) * 2;
        x0 = (g_MenuAltLayout != 0) ? 0x69 : 0xA8;
        y0 = 0x9E;
        callScratch = scratch;
        callX = x0;
        y0 = (s16)(y0 - offset);
        x0 += 0x1C;
        y1 = (s16)(offset + 0x9F);
        GameDrawTexturedQuad(
            callScratch,
            callX,
            y0,
            x0,
            y0,
            callX,
            y1,
            x0,
            y1,
            0xB0,
            0x38,
            0xCC,
            0x38,
            0xB0,
            0x6C,
            0xCC,
            0x6C,
            0x7F,
            0x7F,
            0x7F,
            0x232,
            0,
            0,
            0x1C);
    }

    render1 = g_MenuAltPanelProgressB;
    if (render1 != 0) {
        offset = render1 - 1;
        x0 = (g_MenuAltLayout != 0) ? 0x92 : 0xC0;
        y0 = 0x128;
        callScratch = scratch;
        callX = x0;
        y0 = (s16)(y0 - offset);
        x0 += 0x4E;
        y1 = (s16)(render1 + 0x128);
        GameDrawTexturedQuad(
            callScratch,
            callX,
            y0,
            x0,
            y0,
            callX,
            y1,
            x0,
            y1,
            0x61,
            0x38,
            0xAF,
            0x38,
            0x61,
            0x58,
            0xAF,
            0x58,
            0x7F,
            0x7F,
            0x7F,
            0x259,
            0,
            0,
            0x1C);
    }

    if (step0 > 0) {
        value = g_MenuAltPanelProgressA + step0;
        g_MenuAltPanelProgressA = value;
        if (value >= 0xF) {
            g_MenuAltPanelProgressA = 0xE;
        }
    }

    if (step1 > 0) {
        value = g_MenuAltPanelProgressB + step1;
        g_MenuAltPanelProgressB = value;
        if (value >= 0x11) {
            
            g_MenuAltPanelProgressB = 0x10;
        }
    }
}

void FlipCourseCard(s32 *p0, s32 *p1, s32 *p2) {
    SVec verts[4];
    MenuProjectedVertex out[4];
    Matrix mtx;
    OT_TYPE *scratch2;
    s32 n;
    s32 v;
    s32 depth;

    verts[0] = g_CourseCardVerts[0];
    verts[1] = g_CourseCardVerts[1];
    verts[2] = g_CourseCardVerts[2];
    verts[3] = g_CourseCardVerts[3];

    scratch2 = SCRATCH_OT_BASE_AS(OT_TYPE) + 1;

    n = *p0 - *p1;
    if (n != 0) {
        if (n > 0)
            n = (n + 12) / 12;
        else
            n = (n - 12) / 12;
    }
    *p1 = *p1 + n;
    n = *p1 / 1000;
    if (n < 11) {
        n = 11;
    }
    if (n < 1024) {
        v = *p2;
        if (v >= 0) {
            g_CourseCardFace = v;
            *p2 = -1;
        }
    }

    switch (g_CourseCardFace) {
    case 1:
        depth = 0x1F8;
        break;
    case 2:
        depth = 0x20B;
        break;
    case 3:
        depth = 0x1F9;
        break;
    default:
        return;
    }

    BuildRotMatrixY(&mtx, n);
    ApplyMatrixSV(&mtx, &verts[0], out[0].components);
    ApplyMatrixSV(&mtx, &verts[1], out[1].components);
    ApplyMatrixSV(&mtx, &verts[2], out[2].components);
    ApplyMatrixSV(&mtx, &verts[3], out[3].components);

    {
        s16 x0;
        s16 y0;
        s16 x1;
        s16 y1;
        s16 x2;
        s16 y2;
        s16 x3;
        s16 y3;

        x0 = out[0].position.x;
        x1 = out[1].position.x;
        x2 = out[2].position.x;
        x3 = out[3].position.x;
        y0 = out[0].position.y;
        y1 = out[1].position.y;
        y2 = out[2].position.y;
        y3 = out[3].position.y;

        x0 += 0xE4;
        y0 += 0x58;
        y1 += 0x58;
        x2 += 0xE4;
        y2 += 0x58;
        x3 += 0xE4;
        y3 += 0x58;
        x1 += 0xE4;

        GameDrawTexturedQuad(scratch2,
            x0, y0, x1, y1, x2, y2, x3, y3,
            0xA0, 0x70, 0xDF, 0x70, 0xA0, 0xBF, 0xDF, 0xBF,
            0x7F, 0x7F, 0x7F,
            depth,
            0, 0,
            0x1C);
    }
}

void DrawTimeAttackPlate(s32 stepArg) {
    void *scratch = SCRATCH_OT_BASE;
    s32 step = stepArg;
    s32 value;
    s32 renderValue;
    s32 y0;
    s16 y1;

    if (step == 0) {
        g_TimeAttackPlateProgress = 0;
        return;
    }

    if (step < 0) {
        value = g_TimeAttackPlateProgress + step;
        g_TimeAttackPlateProgress = value;
        if (value < 0) {
            g_TimeAttackPlateProgress = 0;
        }
    }

    renderValue = g_TimeAttackPlateProgress;
    y0 = 0xD7;
    if (renderValue != 0) {
        y0 = (s16)(y0 - renderValue);
        y1 = (s16)(renderValue + 0xD8);
        GameDrawTexturedQuad(
            scratch,
            0x4C,
            y0,
            0x7C,
            y0,
            0x4C,
            y1,
            0x7C,
            y1,
            0xCC,
            0x38,
            0xFC,
            0x38,
            0xCC,
            0x50,
            0xFC,
            0x50,
            0,
            0,
            0,
            0x20F,
            1,
            0,
            0x1C);
    }

    if (step > 0) {
        value = g_TimeAttackPlateProgress + step;
        g_TimeAttackPlateProgress = value;
        if (value >= 0xD) {
            
            g_TimeAttackPlateProgress = 0xC;
        }
    }
}

/* The menu-mode twin of InitTrackLighting. */
void InitMenuLighting(void) {
    g_SceneColorMatrix = g_MenuColorMatrix;
    g_SceneLightMatrix = g_MenuLightMatrix;
    SetColorMatrix(&g_SceneColorMatrix);
    SetLightMatrix(&g_SceneLightMatrix);
    SetBackColor(0x20, 0x20, 0x20);
    SetFarColor(0, 0, 0);
    SetFogNear(0x4E20, 0x140);
}

void InitMenuMode(void) {
    GameRaceProgress *p;

    SetDispMask(0);
    g_MirrorMode = 0;
    p = g_RaceProgress;
    {
        s32 t0 = p->course;
        s32 t1 = p->carIndex;
        s32 t2 = p->classIndex;
        s32 t3 = p->money.value;
        g_FrameSyncThreshold = 0x80;
        g_CourseIndex = t0;
        g_PlayerCarIndex = t1;
        g_GrandPrixClass = t2;
        g_PlayerMoney = t3;
    }
    InitRenderState(1);

    SetupDisplay480(0, 0, 0);
    g_SceneId = 8;
    g_SceneTimer = 0;
    if (g_GrandPrixMode != 0) {
        g_GrandPrixSeries = g_SeriesSelection;
    } else {
        g_GrandPrixSeries = g_RaceProgress->money.half[0];
    }
    g_CourseIndex = (g_GrandPrixSeries << 2) | g_CourseIndex;
    InitMenuLighting();

    SCRATCH_VIEW_X = 0;
    SCRATCH_VIEW_Y = -64;
    SCRATCH_VIEW_Z = -256;
    SCRATCH_VIEW_ANGLE_X = 0x100;
    SCRATCH_VIEW_ANGLE_Y = 0;
    SCRATCH_VIEW_ANGLE_Z = 0;
    SetCameraRotMatrix();
    ScaleMatrix(SCRATCH_VIEW_MATRIX_GTE, &g_MenuViewScale);

    g_CourseSelectModalScript = g_UiEmptyScript;
    g_CarSelectPopupScript = g_UiEmptyScript;
    g_CustomizePopupScript = g_UiEmptyScript;
    g_TeamLogoSubPanelScript = g_UiEmptyScript;
    g_LogoSampleSubPanelScript = g_UiEmptyScript;
    g_CarShopModalScript = g_UiEmptyScript;
    g_EngineerShopModalScript = g_UiEmptyScript;
    g_MenuViewAngle = 500000;
    g_MenuViewAngleTarget = 500000;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    g_MenuHintBarProgress = 0;
    g_MenuConfirmTimer = 0;
    D_8009B304 = 0;
    GameMenuBusy = 0;
    g_MenuHintBarStep = 0;
    g_ClassChangeApplied = 0;
    g_CourseSwapDelay = 0;
    g_MenuViewOffset = 0;
    g_MenuViewOffsetTarget = 0;
    g_CourseCardSpin = 0;
    g_CourseCardSpinTarget = 0;
    g_CourseCardPendingGrade = 0;
    g_MenuPendingCourseIndex = -1;
    g_CarSwapFromIndex = 0;
    g_CarSwapToIndex = -1;
    g_MenuOverlayPattern = 0;
    g_CarNamePlateStep = 0;
    g_MenuPlateCarIndex = 0;
    g_CarSpecGraphStep = 0;
    D_8009B328 = 0;
    g_MenuCourseModelIndex = g_CourseIndex;
    g_MenuAltPanelStep = 0;
    g_MenuAltPanelStep2 = 0;
    g_TimeAttackPlateStep = 0;
    g_MenuHintButtonsVisible = 1;
    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = -1;
    g_MenuAltLayoutSetting = 0;
    g_CarShopUnlockAll = 0;
    g_MenuScreen = 0;
    g_CourseSelectOption = 0;
    g_CarSelectCursor = 0;
    g_RankingOption = 0;
    g_DesignModeOption = 0;
    D_801E4D74 = 0;

    DrawCourseSelectScreen(0);
    DrawRankingScreen(0);
    DrawCarSelectScreen(0);
    DrawCustomizeScreen(0);
    DrawDesignModeScreen(0);
    DrawTeamLogoScreen(0);
    DrawLogoSampleScreen(0);
    DrawTeamNameScreen(0);
    DrawPaintColorScreen(0);
    DrawCarShopScreen(0);
    DrawEngineerShopScreen(0);
    DrawCarSpecGraph(0);
    DrawMenuLightBurst(0);
    DrawTimeAttackPlate(0);
}

/* Counts the enabled entries of g_CarTable. */
s32 CountOwnedCars(void) {
    s32 count = 0;
    s32 i = 0;
    CarEntry *entries = g_CarTable;
    s32 flag;

    do {
        flag = entries[i].enabled;
        if (flag != 0) {
            count++;
        }
        i++;
    } while (i < 0xD);

    return count;
}
