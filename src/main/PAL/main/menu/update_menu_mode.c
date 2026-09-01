#include "game/car.h"
#include "game/menu.h"

void UpdateMenuMode(void) {
    OT_TYPE *ot;
    u32 screenRange;

    ot = RENDER_OT_BASE_AS(OT_TYPE);
    g_AnimTimer++;
    g_SceneTimer++;
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    DrawSolidRect(ot, 0, 0, 0x140, 2, 0, 0, 0, 0xFF);

    screenRange = g_MenuScreen - 1;
    if (screenRange < 2) {
        g_RenderState.otShift = 1;
    } else {
        g_RenderState.otShift = 5;
    }

    if (g_MenuHandlerIndex > 0) {
        g_MenuScreenDraw[g_MenuHandlerIndex](0x14);
    }
    if (g_MenuHandlerIndex2 > 0) {
        g_MenuOutgoingScreenProgress = g_MenuScreenDraw[g_MenuHandlerIndex2](-10);
    }
    g_MenuScreenUpdate[g_MenuScreen]();

    DrawCarSpecGraph(g_CarSpecGraphStep, g_CarTable[(g_MenuScreen == MENU_SCREEN_CAR_SHOP) ? g_CarListCursor : g_PlayerCarIndex].tireCompound);

    if (g_MenuHintBarStep == 0) {
        return;
    }
    if (RunTimedDrawScript(g_MenuHintBarScript, &g_MenuHintBarProgress,
                           g_MenuHintBarStep) == 0) {
        return;
    }

    if (g_MenuHintButtonsVisible != 0) {
        if (g_PadType == 0x23) {
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
