#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"

s32 UpdateMemoryCardFade(void) {
    s32 busy = 0;
    s32 step;

    if (g_SceneTimer == 2) SetDispMask(1);
    if ((u32)g_SceneTimer < 6) {
        DrawMenuFadeOverlay(g_McFadeLevel);
        return 0;
    }
    step = g_McFadeStep;
    if (step < 0) {
        g_McFadeLevel += step;
        busy = 1;
        if (g_McFadeLevel <= 0) {
            g_McFadeStep = 0;
            g_McFadeLevel = 0;
        }
    } else if (step > 0) {
        g_McActionBusy = 1;
        g_McFadeLevel += step;
        busy = 1;
        if (g_McFadeLevel >= 0xFF) {
            g_McFadeStep = 0;
            g_McFadeLevel = 0;
            g_McActionBusy = 0;
            g_SceneId = 2;
        }
    }
    DrawMenuFadeOverlay(g_McFadeLevel);
    return busy;
}

s32 AdvanceMemoryCardMenuStartup(void) {
    s32 next;

    if ((u32)g_SceneTimer >= 5) {
        g_SceneTimer++;
        return 1;
    }
    next = ++g_SceneTimer;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    if (next == 3) {
        g_McSlotUsedMask = 0;
        ClearSaveHeaderRows(g_McSaveHeaders);
        g_McLastMenuState = MC_MENU_STATE_NO_CARD;
        g_McMenuPhase = MC_PROMPT_NONE;
        g_McMenuSelection = MC_MENU_STATE_BUSY;
        g_McMenuState = MC_MENU_STATE_BUSY;
        g_McActionState = 0;
        g_McActionResult = 0;
        g_McConfirmChoice = 0;
        g_McActionTimer = 0;
        g_McActionBusy = 0;
    }
    return 0;
}

void DrawMemoryCardMenu(void) {
    DrawMemoryCardScreen(g_McMenuPage, g_McFromLoadMenu, g_McMenuRowCursor,
                         g_McSlotCursor);
    if (g_McMenuPhase != MC_PROMPT_NONE) {
        DrawMemoryCardMessage(g_McMenuPhase - 1);
    }
    DrawMemoryCardSaveRows(g_McSlotUsedMask, g_McSaveHeaders);
}
