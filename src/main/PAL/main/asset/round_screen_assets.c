#include "game/asset.h"
#include "game/car.h"
#include "game/random.h"
#include "game/race.h"
#include "game/render.h"

enum {
    OVAL_MINIMUM_CLASS = 2,
    ROUND_SCREENS_PER_SERIES = 6
};

static s32 RandomClassInRange(s32 minimum, s32 maximum) {
    if (maximum < minimum) {
        return minimum;
    }
    return minimum + (Random15() & 0xFFF) % (maximum - minimum + 1);
}

static s32 RandomAvailableClass(void) {
    return RandomClassInRange(0, g_MaxClassReached[g_GrandPrixSeries]);
}

s32 RequestRoundAssets(void) {
    if (g_AssetLoadState != 0) {
        ResetAssetLoader();
    }

    if (g_GrandPrixMode == 0) {
        g_GrandPrixClass = RandomAvailableClass();
        if (SeriesCourseIndex() == 3 &&
            g_GrandPrixClass < OVAL_MINIMUM_CLASS) {
            s32 maximum = g_MaxClassReached[g_GrandPrixSeries];
            g_GrandPrixClass =
                RandomClassInRange(OVAL_MINIMUM_CLASS, maximum);
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
    return ASSET_ROUND_SCREEN_BASE +
           g_GrandPrixSeries * ROUND_SCREENS_PER_SERIES +
           g_GrandPrixClass;
}

/* Asset ASSET_VOICE_BANK is written immediately after the round-screen block.
 * Its first three words contain one shared value and two relative offsets. */
static void LoadRoundScreen(void) {
    s32 roundSize = LoadAsset(RoundScreenAssetId(), g_ImageBlockBuffer);

    if (roundSize == 0) {
        return;
    }
    g_AssetBlockPtr2 = g_ImageBlockBuffer + roundSize;
    g_AssetLoadState = 2;
}

static void LoadRoundVoiceBank(void) {
    VoiceBankAssetHeader *header;

    if (LoadAsset(ASSET_VOICE_BANK, g_AssetBlockPtr2) == 0) {
        return;
    }
    header = (VoiceBankAssetHeader *)g_AssetBlockPtr2;
    g_SharedAssetWord0 = header->sharedHeaderSize;
    g_AssetBlockPtr = g_AssetBlockPtr2 + header->audioHeaderOffset;
    g_AssetSubBlockPtr = g_AssetBlockPtr2 + header->audioBodyOffset;
    g_AssetLoadState = 0;
}

void LoadRoundAssets(void) {
    switch (g_AssetLoadState) {
    case 1:
        LoadRoundScreen();
        break;
    case 2:
        LoadRoundVoiceBank();
        break;
    }
}
