#include "game/asset.h"
#include "game/menu.h"
#include "game/memcard.h"
void EnterMemoryCardMenuFromLoad(void) {
    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    if (g_AssetLoadState == 0) {
        UploadImageAsset(g_ImageBlockBuffer);
        g_McMenuRowCursor = 2;
        g_McMenuRowCount = 3;
        g_McMenuState = -1;
        g_SceneTimer = 0;
        g_McMenuPage = 0;
        g_McMenuSubState = 1;
        g_McFromLoadMenu = 1;
        StartMemoryCardEvents();
        g_McFadeStep = -8;
        g_McFadeLevel = 0xFF;
        g_SceneId = 0x1A;
    }
}
