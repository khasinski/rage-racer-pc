#include "game/asset.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/scene_runtime.h"

#include <string.h>

void StartMenuExitFade(MemoryCardSession *memoryCard) {
    StopMemoryCardEvents();
    memoryCard->fadeStep = 8;
}

static void ResetMemoryCardMenuSession(MemoryCardSession *memoryCard) {
    memoryCard->poll.state = MC_STATUS_REQUEST_INFO;
    memoryCard->poll.ticks = 0;
    memoryCard->poll.result = MC_CARD_RESULT_PENDING;
    memoryCard->poll.pendingResult = MC_CARD_RESULT_PENDING;
    memoryCard->poll.lastStatus = MC_CARD_RESULT_PENDING;
    memoryCard->noCardTicks = 0;
    memoryCard->errorTicks = 0;
    memoryCard->errorPending = 0;
    memoryCard->errorCountdown = 3;
    memoryCard->settleTicks = 0;
}

static void InitializeMemoryCardMenu(MemoryCardSession *memoryCard,
                                     s32 fromLoadMenu) {
    memoryCard->menuRow = fromLoadMenu != 0 ? 2 : 0;
    memoryCard->menuState = MC_MENU_STATE_NO_CARD;
    g_SceneTimer = 0;
    memoryCard->menuPage = 0;
    memoryCard->fromLoadMenu = fromLoadMenu;
    memset(&memoryCard->action, 0, sizeof(memoryCard->action));
    ResetMemoryCardMenuSession(memoryCard);
    StartMemoryCardEvents();
    memoryCard->fadeStep = -8;
    memoryCard->fadeLevel = 0xFF;
    g_SceneId = 0x1A;
}

void EnterMemoryCardMenu(void) {
    MemoryCardSession *memoryCard = SceneRuntimeMemoryCard();

    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    InitializeMemoryCardMenu(memoryCard, 0);
}

void EnterMemoryCardMenuFromLoad(void) {
    MemoryCardSession *memoryCard = SceneRuntimeMemoryCard();

    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    if (!AssetLoadCompletedSuccessfully()) return;

    if (!UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                          g_ImageBlockSize)) {
        return;
    }
    InitializeMemoryCardMenu(memoryCard, 1);
}
