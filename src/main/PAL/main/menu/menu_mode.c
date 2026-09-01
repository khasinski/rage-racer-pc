#include "game/car.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/render_internal.h"


void DrawMenuAltPanel(s32 stepA, s32 stepB) {
    void *ot = RENDER_OT_BASE;
    s32 value;
    s32 offset;
    s32 x0;
    s32 y0;
    s32 render1;
    s16 y1;
    void *savedOt;
    s32 callX;

    if (stepA == 0 && stepB == 0) {
        g_MenuAltPanelProgressA = 0;
        g_MenuAltPanelProgressB = 0;
        return;
    }

    if (stepA < 0) {
        value = g_MenuAltPanelProgressA + stepA;
        g_MenuAltPanelProgressA = value;
        if (value < 0) {
            g_MenuAltPanelProgressA = 0;
        }
    }

    if (stepB < 0) {
        value = g_MenuAltPanelProgressB + stepB;
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
        savedOt = ot;
        callX = x0;
        y0 = (s16)(y0 - offset);
        x0 += 0x1C;
        y1 = (s16)(offset + 0x9F);
        GameDrawTexturedQuad(
            savedOt,
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
        savedOt = ot;
        callX = x0;
        y0 = (s16)(y0 - offset);
        x0 += 0x4E;
        y1 = (s16)(render1 + 0x128);
        GameDrawTexturedQuad(
            savedOt,
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

    if (stepA > 0) {
        value = g_MenuAltPanelProgressA + stepA;
        g_MenuAltPanelProgressA = value;
        if (value >= 0xF) {
            g_MenuAltPanelProgressA = 0xE;
        }
    }

    if (stepB > 0) {
        value = g_MenuAltPanelProgressB + stepB;
        g_MenuAltPanelProgressB = value;
        if (value >= 0x11) {
            g_MenuAltPanelProgressB = 0x10;
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
    SetDispMask(0);
    g_MirrorMode = 0;
    g_FrameSyncThreshold = 0x80;
    g_CourseIndex = g_RaceProgress->course;
    g_PlayerCarIndex = g_RaceProgress->carIndex;
    g_GrandPrixClass = g_RaceProgress->classIndex;
    g_PlayerMoney = g_RaceProgress->money.value;
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

    g_RenderState.viewX = 0;
    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_RenderState.viewAngleX = 0x100;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;
    SetCameraRotMatrix();
    ScaleMatrix((&g_RenderState.matrix), &g_MenuViewScale);

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
    DrawCarSpecGraph(0, 0); /* step 0 resets and returns before the grade */
    DrawMenuLightBurst(0);
    DrawTimeAttackPlate(0);
}

/* Counts the enabled entries of g_CarTable. */
s32 CountOwnedCars(void) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 0xD; i++) {
        if (g_CarTable[i].enabled != 0) {
            count++;
        }
    }

    return count;
}
