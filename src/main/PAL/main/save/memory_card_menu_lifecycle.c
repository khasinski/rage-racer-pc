#include "game/asset.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/scene_runtime.h"

#include <string.h>

void StartMenuExitFade(void) {
    StopMemoryCardEvents();
    g_McFadeStep = 8;
}

static void ResetMemoryCardMenuSession(MemoryCardPoll *poll) {
    poll->state = MC_STATUS_REQUEST_INFO;
    poll->ticks = 0;
    poll->result = MC_CARD_RESULT_PENDING;
    poll->pendingResult = MC_CARD_RESULT_PENDING;
    poll->lastStatus = MC_CARD_RESULT_PENDING;
    g_McNoCardTicks = 0;
    g_McErrorTicks = 0;
    g_McErrorPending = 0;
    g_McErrorCountdown = 3;
    g_McSettleTicks = 0;
}

static void InitializeMemoryCardMenu(MemoryCardAction *action,
                                     MemoryCardPoll *poll,
                                     s32 fromLoadMenu) {
    g_McMenuRowCursor = fromLoadMenu != 0 ? 2 : 0;
    g_McMenuState = MC_MENU_STATE_NO_CARD;
    g_SceneTimer = 0;
    g_McMenuPage = 0;
    g_McFromLoadMenu = fromLoadMenu;
    memset(action, 0, sizeof(*action));
    ResetMemoryCardMenuSession(poll);
    StartMemoryCardEvents();
    g_McFadeStep = -8;
    g_McFadeLevel = 0xFF;
    g_SceneId = 0x1A;
}

void EnterMemoryCardMenu(void) {
    MemoryCardSession *memoryCard = SceneRuntimeMemoryCard();

    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    InitializeMemoryCardMenu(&memoryCard->action, &memoryCard->poll, 0);
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
    InitializeMemoryCardMenu(&memoryCard->action, &memoryCard->poll, 1);
}
