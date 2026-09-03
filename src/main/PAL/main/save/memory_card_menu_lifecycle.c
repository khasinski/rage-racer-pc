#include "game/asset.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"

void DrawMenuFadeOverlay(s32 level) {
    DrawFullscreenFadeTile480(level, 0x40);
}

void StartMenuExitFade(void) {
    StopMemoryCardEvents();
    g_McFadeStep = 8;
}

static void ResetMemoryCardMenuSession(void) {
    g_McStatusState = MC_STATUS_REQUEST_INFO;
    g_McPollTicks = 0;
    g_McStatusResult = MC_CARD_RESULT_PENDING;
    g_McPollStatus = MC_CARD_RESULT_PENDING;
    g_McLastCardStatus = MC_CARD_RESULT_PENDING;
    g_McNoCardTicks = 0;
    g_McErrorTicks = 0;
    g_McErrorPending = 0;
    g_McErrorCountdown = 3;
    g_McSettleTicks = 0;
    g_McCardOkFrames = 0;
}

static void InitializeMemoryCardMenu(s32 fromLoadMenu) {
    g_McMenuRowCount = fromLoadMenu != 0 ? 3 : 2;
    g_McMenuRowCursor = fromLoadMenu != 0 ? 2 : 0;
    g_McMenuState = MC_MENU_STATE_NO_CARD;
    g_SceneTimer = 0;
    g_McMenuPage = 0;
    g_McFromLoadMenu = fromLoadMenu;
    ResetMemoryCardMenuSession();
    StartMemoryCardEvents();
    g_McFadeStep = -8;
    g_McFadeLevel = 0xFF;
    g_SceneId = 0x1A;
}

void EnterMemoryCardMenu(void) {
    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    InitializeMemoryCardMenu(0);
}

void EnterMemoryCardMenuFromLoad(void) {
    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    if (!AssetLoadCompletedSuccessfully()) return;

    if (!UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                          g_ImageBlockSize)) {
        return;
    }
    InitializeMemoryCardMenu(1);
}
