#include "game/asset.h"
#include "game/menu.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
void EnterMemoryCardMenuFromLoad(void) {
    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    if (g_AssetLoadState == 0) {
        UploadImageAsset(g_ImageBlockBuffer);
        g_McMenuRowCursor = 2;
        g_McMenuRowCount = 3;
        g_McMenuState = MC_MENU_STATE_NO_CARD;
        g_SceneTimer = 0;
        g_McMenuPage = 0;
        g_McFromLoadMenu = 1;
        StartMemoryCardEvents();
        g_McFadeStep = -8;
        g_McFadeLevel = 0xFF;
        g_SceneId = 0x1A;
    }
}
