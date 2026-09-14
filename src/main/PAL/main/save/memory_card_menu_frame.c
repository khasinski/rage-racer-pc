#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"

s32 UpdateMemoryCardFade(MemoryCardAction *action) {
    const s32 step = g_McFadeStep;

    if (g_SceneTimer == 2) SetDispMask(1);
    if ((u32)g_SceneTimer < 6) {
        DrawFullscreenFadeTile480(g_McFadeLevel, 0x40);
        return 0;
    }
    g_McFadeLevel = StepFade(g_McFadeLevel, step, 0xFF);
    if (step < 0 && g_McFadeLevel == 0) {
        g_McFadeStep = 0;
    } else if (step > 0) {
        action->busy = 1;
        if (g_McFadeLevel == 0xFF) {
            g_McFadeStep = 0;
            g_McFadeLevel = 0;
            action->busy = 0;
            g_SceneId = 2;
        }
    }
    DrawFullscreenFadeTile480(g_McFadeLevel, 0x40);
    return step != 0;
}

s32 AdvanceMemoryCardMenuStartup(MemoryCardAction *action,
                                 MemoryCardSlots *slots) {
    s32 next;

    if ((u32)g_SceneTimer >= 5) {
        g_SceneTimer++;
        return 1;
    }
    next = ++g_SceneTimer;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    if (next == 3) {
        slots->usedMask = 0;
        ClearSaveHeaderRows(slots->headers);
        g_McLastMenuState = MC_MENU_STATE_NO_CARD;
        g_McMenuPhase = MC_PROMPT_NONE;
        g_McMenuSelection = MC_MENU_STATE_BUSY;
        g_McMenuState = MC_MENU_STATE_BUSY;
        action->state = 0;
        action->result = 0;
        action->confirmChoice = 0;
        action->timer = 0;
        action->busy = 0;
    }
    return 0;
}

void DrawMemoryCardMenu(const MemoryCardSlots *slots) {
    DrawMemoryCardScreen(g_McMenuPage, g_McFromLoadMenu, g_McMenuRowCursor,
                         g_McSlotCursor);
    if (g_McMenuPhase != MC_PROMPT_NONE) {
        DrawMemoryCardMessage(g_McMenuPhase - 1);
    }
    DrawMemoryCardSaveRows(slots->usedMask, slots->headers);
}
