#include "common.h"
#include "game/game_input.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_flow.h"
#include "game/render.h"
#include "game/render_workspace.h"
#include "game/state.h"
#include "psyq/gpu.h"
#include "game/menu_context.h"

void UpdateMenuMode(void) {
    OT_TYPE *scratch;
    s32 c0;
    s32 c1;
    u32 screenRange;
    const MenuRuntime *runtime;

    c0 = g_AnimTimer;
    c1 = g_SceneTimer;
    scratch = RENDER_OT_BASE_AS(OT_TYPE);
    c0 += 1;
    c1 += 1;
    g_AnimTimer = c0;
    g_SceneTimer = c1;
    if (c1 == 2) {
        SetDispMask(1);
    }
    DrawSolidRect(scratch, 0, 0, 0x140, 2, 0, 0, 0, 0xFF);

    runtime = MenuFlowGetRuntime();
    screenRange = runtime->activeScreen - 1;
    if (screenRange < 2) {
        RENDER_OT_SHIFT = 1;
    } else {
        RENDER_OT_SHIFT = 5;
    }

    if (runtime->incomingScreen > 0) {
        g_MenuScreenDraw[runtime->incomingScreen](0x14);
    }
    if (runtime->outgoingScreen > 0) {
        g_MenuOutgoingScreenProgress =
            g_MenuScreenDraw[runtime->outgoingScreen](-10);
    }
    g_MenuScreenUpdate[runtime->activeScreen]();

    DrawCarSpecGraph(g_CarSpecGraphStep,
        g_CarTable[(runtime->activeScreen == MENU_SCREEN_CAR_SHOP)
            ? g_CarListCursor : g_PlayerCarIndex].tireCompound);

    {
        register s32 flag asm("$6");
        flag = g_MenuHintBarStep;
        if (flag == 0) {
            MenuFlowPublishLegacyState();
            return;
        }
    }
    if (RunTimedDrawScript(&g_MenuHintBarScript, &g_MenuHintBarProgress,
                           g_MenuHintBarStep) == 0) {
        MenuFlowPublishLegacyState();
        return;
    }

    if (g_MenuHintButtonsVisible != 0) {
        if (g_GameInput.controllerType == 0x23) {
            scratch++;
            DrawSprite(scratch, 0xC0, 0x1A1, 0x20, 0xC, 0x94, 0xF4, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(scratch, 0xF0, 0x1A1, 0x2C, 0xC, 0xB4, 0xF4, 0, 0, 0, 0x244, 1, 1, 0x3B);
        } else {
            scratch++;
            DrawSprite(scratch, 0xC0, 0x1A1, 0x20, 0xC, 0x94, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(scratch, 0xF0, 0x1A1, 0x2C, 0xC, 0xB4, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3B);
        }
    }
    DrawBitPatternOverlay(g_MenuOverlayPattern);
    MenuFlowPublishLegacyState();
}
