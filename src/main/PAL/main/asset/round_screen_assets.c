#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/random.h"
#include "game/race.h"
#include "game/render.h"

enum {
    OVAL_MINIMUM_CLASS = 2,
    ROUND_SCREENS_PER_SERIES = GRAND_PRIX_PRIZE_CLASS_COUNT,
    ROUND_MAX_CLASS = ROUND_SCREENS_PER_SERIES - 1,
    ROUND_LOAD_SCREEN = 1,
    ROUND_LOAD_VOICE_BANK = 2,
};

static s32 RandomClassInRange(s32 minimum, s32 maximum) {
    if (maximum < minimum) {
        return minimum;
    }
    return minimum + (Random15() & 0xFFF) % (maximum - minimum + 1);
}

static s32 MaximumUnlockedRoundClass(void) {
    s32 series = g_GrandPrixSeries;
    s32 maximum;

    if ((u32)series >= GRAND_PRIX_SERIES_COUNT) {
        series = 0;
    }
    maximum = g_MaxClassReached[series];
    if (maximum < 0) return 0;
    return maximum < ROUND_MAX_CLASS ? maximum : ROUND_MAX_CLASS;
}

s32 RequestRoundAssets(void) {
    if (g_AssetLoadState != 0) {
        ResetAssetLoader();
    }

    if (g_GrandPrixMode == 0) {
        s32 maximum = MaximumUnlockedRoundClass();

        g_GrandPrixClass = RandomClassInRange(0, maximum);
        if (SeriesCourseIndex() == 3 &&
            g_GrandPrixClass < OVAL_MINIMUM_CLASS) {
            g_GrandPrixClass =
                RandomClassInRange(OVAL_MINIMUM_CLASS, maximum);
        }
    }

    if (g_AssetRequestType == ASSET_REQUEST_ROUND_SCREEN) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return g_AssetLoadFailed != 0 ? -1 : 0;
    }

    g_AssetRequestType = ASSET_REQUEST_ROUND_SCREEN;
    g_AssetLoadFailed = 0;
    g_AssetLoadState = ROUND_LOAD_SCREEN;
    return 1;
}

static s32 RoundScreenAssetId(void) {
    if (g_GrandPrixMode == 0) {
        return ASSET_TIME_ATTACK_ROUND_SCREEN;
    }
    if ((u32)g_GrandPrixSeries >= GRAND_PRIX_SERIES_COUNT ||
        (u32)g_GrandPrixClass >= ROUND_SCREENS_PER_SERIES) {
        return -1;
    }
    return ASSET_ROUND_SCREEN_BASE +
           g_GrandPrixSeries * ROUND_SCREENS_PER_SERIES +
           g_GrandPrixClass;
}

/* Asset ASSET_VOICE_BANK is written immediately after the round-screen block.
 * Its first three words contain one shared value and two relative offsets. */
static void LoadRoundScreen(void) {
    s32 assetId = RoundScreenAssetId();
    s32 roundSize;

    if (assetId < 0) {
        FailAssetLoad();
        return;
    }
    roundSize = LoadAsset(assetId, g_ImageBlockBuffer);

    if (roundSize <= 0) return;

    g_ImageBlockSize = (size_t)roundSize;
    g_AssetBlockPtr2 = g_ImageBlockBuffer + roundSize;
    g_AssetLoadState = ROUND_LOAD_VOICE_BANK;
}

static void LoadRoundVoiceBank(void) {
    const VoiceBankAssetHeader *header;
    s32 loadedSize;

    loadedSize = LoadAsset(ASSET_VOICE_BANK, g_AssetBlockPtr2);
    if (loadedSize == 0) return;

    header = (const VoiceBankAssetHeader *)(const void *)g_AssetBlockPtr2;
    if (loadedSize < (s32)sizeof(*header) || header->sharedHeaderSize < 0 ||
        header->audioHeaderOffset < (s32)sizeof(*header) ||
        header->audioHeaderOffset > loadedSize ||
        header->sharedHeaderSize >
            loadedSize - header->audioHeaderOffset ||
        header->audioBodyOffset !=
            header->audioHeaderOffset + header->sharedHeaderSize ||
        header->audioBodyOffset >= loadedSize) {
        FailAssetLoad();
        return;
    }
    g_RaceVoiceHeaderSize = header->sharedHeaderSize;
    g_AssetBlockPtr = g_AssetBlockPtr2 + header->audioHeaderOffset;
    g_AssetBlockSize = (size_t)header->sharedHeaderSize;
    g_AssetSubBlockPtr = g_AssetBlockPtr2 + header->audioBodyOffset;
    g_AssetSubBlockSize =
        (size_t)(loadedSize - header->audioBodyOffset);
    g_AssetLoadState = 0;
}

void LoadRoundAssets(void) {
    switch (g_AssetLoadState) {
    case ROUND_LOAD_SCREEN:
        LoadRoundScreen();
        break;
    case ROUND_LOAD_VOICE_BANK:
        LoadRoundVoiceBank();
        break;
    }
}
