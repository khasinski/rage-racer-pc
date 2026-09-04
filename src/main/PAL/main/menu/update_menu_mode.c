#include "game/car.h"
#include "game/menu.h"
#include "game/state.h"

static u32 CurrentMenuCarTireCompound(void) {
    s32 carIndex = g_MenuScreen == MENU_SCREEN_CAR_SHOP
                       ? g_CarListCursor
                       : g_PlayerCarIndex;

    if (g_CarTable == NULL || (u32)carIndex >= GAME_CAR_COUNT) {
        return 0;
    }
    return g_CarTable[carIndex].tireCompound;
}

void UpdateMenuMode(void) {
    GameOrderingTableEntry *ot;

    ot = RENDER_OT_BASE;
    g_AnimTimer++;
    g_SceneTimer++;
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    DrawSolidRect(ot, 0, 0, 0x140, 2, 0, 0, 0, 0xFF);

    if ((u32)g_MenuScreen >= MENU_SCREEN_COUNT) {
        g_MenuScreen = MENU_SCREEN_BOOTSTRAP;
    }
    if (g_MenuScreen == MENU_SCREEN_COURSE_SELECT ||
        g_MenuScreen == MENU_SCREEN_RANKING) {
        g_RenderState.otShift = 1;
    } else {
        g_RenderState.otShift = 5;
    }

    if (g_MenuHandlerIndex > MENU_SCREEN_BOOTSTRAP &&
        g_MenuHandlerIndex < MENU_SCREEN_COUNT) {
        g_MenuScreenDraw[g_MenuHandlerIndex](0x14);
    }
    if (g_MenuOutgoingHandlerIndex > MENU_SCREEN_BOOTSTRAP &&
        g_MenuOutgoingHandlerIndex < MENU_SCREEN_COUNT) {
        g_MenuOutgoingScreenProgress = g_MenuScreenDraw[g_MenuOutgoingHandlerIndex](-10);
    }
    g_MenuScreenUpdate[g_MenuScreen]();

    DrawCarSpecGraph(g_CarSpecGraphStep, CurrentMenuCarTireCompound());

    if (g_MenuHintBarStep == 0) {
        return;
    }
    if (RunTimedDrawScript(g_MenuHintBarScript, &g_MenuHintBarProgress,
                           g_MenuHintBarStep) == 0) {
        return;
    }

    if (g_MenuHintButtonsVisible != 0) {
        if (g_PadType == PAD_TYPE_NEGCON) {
            ot++;
            DrawSprite(ot, 0xC0, 0x1A1, 0x20, 0xC, 0x94, 0xF4, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0xF0, 0x1A1, 0x2C, 0xC, 0xB4, 0xF4, 0, 0, 0, 0x244, 1, 1, 0x3B);
        } else {
            ot++;
            DrawSprite(ot, 0xC0, 0x1A1, 0x20, 0xC, 0x94, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0xF0, 0x1A1, 0x2C, 0xC, 0xB4, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3B);
        }
    }
    DrawBitPatternOverlay(g_MenuOverlayPattern);
}
