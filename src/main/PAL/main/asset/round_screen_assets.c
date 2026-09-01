#include "game/asset.h"
#include "game/car.h"
#include "game/random.h"
#include "game/race.h"
#include "game/render.h"

static s32 RandomAvailableClass(void) {
    s32 maximum = g_MaxClassReached[g_GrandPrixSeries];
    return (Random15() & 0xFFF) % (maximum + 1);
}

s32 RequestRoundAssets(void) {
    if (g_AssetLoadState != 0) {
        ResetAssetLoader();
    }

    if (g_GrandPrixMode == 0) {
        g_GrandPrixClass = RandomAvailableClass();
        if (SeriesCourseIndex() == 3 && g_GrandPrixClass < 2) {
            s32 maximum = g_MaxClassReached[g_GrandPrixSeries];
            g_GrandPrixClass = (Random15() & 0xFFF) % (maximum - 1) + 2;
        }
    }

    if (g_AssetRequestType == ASSET_REQUEST_ROUND_SCREEN) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    g_AssetRequestType = ASSET_REQUEST_ROUND_SCREEN;
    g_AssetLoadState = 1;
    return 1;
}

static s32 RoundScreenAssetId(void) {
    if (g_GrandPrixMode == 0) {
        return ASSET_TIME_ATTACK_ROUND_SCREEN;
    }
    return ASSET_ROUND_SCREEN_BASE + g_GrandPrixSeries * 6 +
           g_GrandPrixClass;
}

/* Asset ASSET_VOICE_BANK is written immediately after the round-screen block.
 * Its first three words contain one shared value and two relative offsets. */
void LoadRoundAssets(void) {
    if (g_AssetLoadState == 1) {
        s32 roundSize = LoadAsset(RoundScreenAssetId(), g_ImageBlockBuffer);
        if (roundSize != 0) {
            g_AssetBlockPtr2 = g_ImageBlockBuffer + roundSize;
            g_AssetLoadState = 2;
        }
    } else if (g_AssetLoadState == 2 &&
               LoadAsset(ASSET_VOICE_BANK, g_AssetBlockPtr2) != 0) {
        s32 *header = GetAssetWords(g_AssetBlockPtr2);

        g_SharedAssetWord0 = header[0];
        g_AssetBlockPtr = g_AssetBlockPtr2 + header[1];
        g_AssetSubBlockPtr = g_AssetBlockPtr2 + header[2];
        g_AssetLoadState = 0;
    }
}
