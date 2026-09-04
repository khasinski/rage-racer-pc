#include "game/car.h"
#include "game/menu.h"
#include "game/state.h"

enum {
    MENU_ACTIVE_SCREEN_FADE_STEP = 20,
    MENU_OUTGOING_SCREEN_FADE_STEP = -10,
    MENU_NEAR_OT_SHIFT = 1,
    MENU_DEFAULT_OT_SHIFT = 5,
};

static u32 CurrentMenuCarTireCompound(void) {
    s32 carIndex = g_MenuScreen == MENU_SCREEN_CAR_SHOP
                       ? g_CarListCursor
                       : g_PlayerCarIndex;

    if (g_CarTable == NULL || (u32)carIndex >= GAME_CAR_COUNT) {
        return 0;
    }
    return g_CarTable[carIndex].tireCompound;
}

static void DrawMenuTransitions(void) {
    if (g_MenuHandlerIndex > MENU_SCREEN_BOOTSTRAP &&
        g_MenuHandlerIndex < MENU_SCREEN_COUNT) {
        g_MenuScreenDraw[g_MenuHandlerIndex](MENU_ACTIVE_SCREEN_FADE_STEP);
    }
    if (g_MenuOutgoingHandlerIndex > MENU_SCREEN_BOOTSTRAP &&
        g_MenuOutgoingHandlerIndex < MENU_SCREEN_COUNT) {
        g_MenuOutgoingScreenProgress =
            g_MenuScreenDraw[g_MenuOutgoingHandlerIndex](
                MENU_OUTGOING_SCREEN_FADE_STEP);
    }
}

static void DrawMenuHints(GameOrderingTableEntry *ot) {
    if (g_MenuHintBarStep == 0 ||
        RunTimedDrawScript(g_MenuHintBarScript, &g_MenuHintBarProgress,
                           g_MenuHintBarStep) == 0) {
        return;
    }

    if (g_MenuHintButtonsVisible != 0 && ot != NULL) {
        GameOrderingTableEntry *hintOt = ot + 1;
        u16 textureV = g_PadType == PAD_TYPE_NEGCON ? 0xF4 : 0xE8;

        DrawSprite(hintOt, 0xC0, 0x1A1, 0x20, 0xC, 0x94, textureV, 0, 0,
                   0, 0x244, 1, 1, 0x3B);
        DrawSprite(hintOt, 0xF0, 0x1A1, 0x2C, 0xC, 0xB4, textureV, 0, 0,
                   0, 0x244, 1, 1, 0x3B);
    }
    DrawBitPatternOverlay(g_MenuOverlayPattern);
}

void UpdateMenuMode(void) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    g_SceneTimer = (s32)((u32)g_SceneTimer + 1u);
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    if (ot != NULL) {
        DrawSolidRect(ot, 0, 0, 0x140, 2, 0, 0, 0, 0xFF);
    }

    if ((u32)g_MenuScreen >= MENU_SCREEN_COUNT) {
        g_MenuScreen = MENU_SCREEN_BOOTSTRAP;
    }
    if (g_MenuScreen == MENU_SCREEN_COURSE_SELECT ||
        g_MenuScreen == MENU_SCREEN_RANKING) {
        g_RenderState.otShift = MENU_NEAR_OT_SHIFT;
    } else {
        g_RenderState.otShift = MENU_DEFAULT_OT_SHIFT;
    }

    DrawMenuTransitions();
    g_MenuScreenUpdate[g_MenuScreen]();
    DrawCarSpecGraph(g_CarSpecGraphStep, CurrentMenuCarTireCompound());
    DrawMenuHints(ot);
}
