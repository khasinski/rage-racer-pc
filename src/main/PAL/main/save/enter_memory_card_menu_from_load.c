#include "common.h"
#include "game/asset.h"
#include "game/state.h"
#include "game/menu.h"
#include "psyq/gpu.h"
#include "game/render.h"
#include "game/memcard.h"
#include "game/game_context.h"
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
        GameSceneSet(SCENE_MEMORY_CARD);
    }
}
