#include "game/memcard.h"
#include "game/menu.h"

void DrawMenuFadeOverlay(s32 level) {
    DrawFullscreenFadeTile480(level, 0x40);
}

void StartMenuExitFade(void) {
    StopMemoryCardEvents();
    g_McFadeStep = 8;
}

void EnterMemoryCardMenu(void) {
    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    g_McMenuRowCount = 2;
    g_McMenuState = -1;
    g_SceneTimer = 0;
    g_McMenuPage = 0;
    g_McMenuRowCursor = 0;
    g_McFromLoadMenu = 0;
    StartMemoryCardEvents();
    g_McFadeStep = -8;
    g_McFadeLevel = 0xFF;
    g_SceneId = 0x1A;
}
