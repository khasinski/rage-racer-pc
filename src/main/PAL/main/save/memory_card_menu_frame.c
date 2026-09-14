#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"

#include <string.h>

s32 UpdateMemoryCardFade(MemoryCardSession *memoryCard) {
    const s32 step = memoryCard->fadeStep;

    if (g_SceneTimer == 2) SetDispMask(1);
    if ((u32)g_SceneTimer < 6) {
        DrawFullscreenFadeTile480(memoryCard->fadeLevel, 0x40);
        return 0;
    }
    memoryCard->fadeLevel = StepFade(memoryCard->fadeLevel, step, 0xFF);
    if (step < 0 && memoryCard->fadeLevel == 0) {
        memoryCard->fadeStep = 0;
    } else if (step > 0) {
        memoryCard->action.busy = 1;
        if (memoryCard->fadeLevel == 0xFF) {
            memoryCard->fadeStep = 0;
            memoryCard->fadeLevel = 0;
            memoryCard->action.busy = 0;
            g_SceneId = 2;
        }
    }
    DrawFullscreenFadeTile480(memoryCard->fadeLevel, 0x40);
    return step != 0;
}

s32 AdvanceMemoryCardMenuStartup(MemoryCardSession *memoryCard) {
    s32 next;

    if ((u32)g_SceneTimer >= 5) {
        g_SceneTimer++;
        return 1;
    }
    next = ++g_SceneTimer;
    memoryCard->phase = MC_PROMPT_ACCESSING;
    if (next == 3) {
        memoryCard->slots.usedMask = 0;
        ClearSaveHeaderRows(memoryCard->slots.headers);
        memoryCard->lastMenuState = MC_MENU_STATE_NO_CARD;
        memoryCard->phase = MC_PROMPT_NONE;
        memoryCard->menuSelection = MC_MENU_STATE_BUSY;
        memoryCard->menuState = MC_MENU_STATE_BUSY;
        memset(&memoryCard->action, 0, sizeof(memoryCard->action));
    }
    return 0;
}

void DrawMemoryCardMenu(const MemoryCardSession *memoryCard) {
    DrawMemoryCardScreen(memoryCard->menuPage, memoryCard->fromLoadMenu, memoryCard->menuRow,
                         memoryCard->slot);
    if (memoryCard->phase != MC_PROMPT_NONE) {
        DrawMemoryCardMessage(memoryCard->phase - 1);
    }
    DrawMemoryCardSaveRows(memoryCard->slots.usedMask,
                           memoryCard->slots.headers, memoryCard->freeBlocks,
                           memoryCard->menuPage, memoryCard->menuRow);
}
