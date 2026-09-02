#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/cd.h"
#include "game/audio.h"
#include "game/menu_internal.h"
#include "game/race.h"

enum {
    BOOT_LOAD_TITLE_SCREEN = 1,
    BOOT_LOAD_AUDIO_HEADER,
    BOOT_LOAD_AUDIO_BODY,
    BOOT_WAIT_FOR_AUDIO,
    BOOT_LOAD_RESOURCES,
    BOOT_LOAD_CAR_SCREEN,
};

enum {
    SAVE_SCREEN_LOAD_ASSET = 1,
    SELECT_BGM_CLOSE_AUDIO = 1,
    SELECT_BGM_LOAD_ASSET = 2,
};

typedef struct SelectBgmAssetHeader {
    s32 audioHeaderOffset;
    s32 sequenceOffset;
    s32 audioBodyOffset;
} SelectBgmAssetHeader;

static void LoadBootTitleScreen(void) {
    u8 *base = GetAssetBytes(g_LoadBuffer);
    s32 loadedSize = LoadAsset(ASSET_TITLE_SCREEN, base);

    if (loadedSize == 0) return;
    g_LoadBufferImageSize = (size_t)loadedSize;
    if (!UploadLoadBufferImage()) {
        FailAssetLoad();
        return;
    }
    g_AssetBlockPtr = base + loadedSize;
    g_AssetLoadState = BOOT_LOAD_AUDIO_HEADER;
}

static void LoadBootAudioHeader(void) {
    s32 loadedSize = LoadAsset(ASSET_BOOT_AUDIO_HEADER, g_AssetBlockPtr);

    if (loadedSize == 0) return;
    g_AssetBlockSize = (size_t)loadedSize;
    g_AssetLoadCursor = g_AssetBlockPtr + loadedSize;
    g_AssetLoadState = BOOT_LOAD_AUDIO_BODY;
}

static void LoadBootAudioBody(void) {
    AudioSlotAsset asset;
    s32 loadedSize = LoadAsset(ASSET_BOOT_AUDIO_BODY, g_AssetLoadCursor);

    if (loadedSize == 0) return;
    asset = (AudioSlotAsset){
        .vabHeader = g_AssetBlockPtr,
        .vabHeaderSize = g_AssetBlockSize,
        .vabBody = g_AssetLoadCursor,
        .vabBodySize = (size_t)loadedSize,
    };
    if (StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) < 0) {
        FailAssetLoad();
        return;
    }
    g_AssetLoadState = BOOT_WAIT_FOR_AUDIO;
}

static void WaitForBootAudio(void) {
    if (PollAudioSlotLoad() != 0) {
        g_AssetLoadState = BOOT_LOAD_RESOURCES;
    }
}

static void LoadBootResources(void) {
    s32 loadedSize = LoadAsset(ASSET_BOOT_RESOURCES, g_AssetLoadCursor);

    if (loadedSize == 0) return;
    g_AssetLoadCursor += loadedSize;
    g_AssetLoadState = BOOT_LOAD_CAR_SCREEN;
}

static void LoadBootCarScreen(void) {
    u8 *assetBase = g_AssetLoadCursor;
    s32 loadedSize;

    loadedSize = LoadAsset(ASSET_BOOT_CAR_SCREEN, assetBase);
    if (loadedSize == 0) return;
    if (!UploadImageAsset(GetImageAssetHeaderWords(assetBase),
                          (size_t)loadedSize)) {
        FailAssetLoad();
        return;
    }
    StoreImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    StoreImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    DrawSync(0);
    g_TeamLogoClut[0] = 0;
    g_AssetBase = assetBase;
    g_AssetLoadState = 0;
}

void LoadBootAssets(void) {
    switch (g_AssetLoadState) {
    case BOOT_LOAD_TITLE_SCREEN:
        LoadBootTitleScreen();
        break;
    case BOOT_LOAD_AUDIO_HEADER:
        LoadBootAudioHeader();
        break;
    case BOOT_LOAD_AUDIO_BODY:
        LoadBootAudioBody();
        break;
    case BOOT_WAIT_FOR_AUDIO:
        WaitForBootAudio();
        break;
    case BOOT_LOAD_RESOURCES:
        LoadBootResources();
        break;
    case BOOT_LOAD_CAR_SCREEN:
        LoadBootCarScreen();
        break;
    }
}

s32 RequestSaveScreenAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_SAVE_SCREEN,
                            SAVE_SCREEN_LOAD_ASSET, 1);
}

void LoadSaveScreenAssets(void) {
    s32 loadedSize;

    if (g_AssetLoadState != SAVE_SCREEN_LOAD_ASSET) return;
    loadedSize = LoadAsset(ASSET_SAVE_SCREEN, g_AssetBase);
    if (loadedSize == 0) return;

    g_AssetLoadState = 0;
    g_ImageBlockBuffer = g_AssetBase;
    g_ImageBlockSize = (size_t)loadedSize;
}

s32 RequestSelectBgmAssetsKeepAudioSlots(void) {
    return RequestAssetLoad(ASSET_REQUEST_SELECT_BGM,
                            SELECT_BGM_LOAD_ASSET, 1);
}

s32 RequestSelectBgmAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_SELECT_BGM,
                            SELECT_BGM_CLOSE_AUDIO, 1);
}

static void LoadSelectBgmAssetPack(void) {
    SelectBgmAssetHeader *header;
    s32 loadedSize;

    loadedSize = LoadAsset(ASSET_SELECT_BGM, g_AssetBase);
    if (loadedSize == 0) return;

    header = (SelectBgmAssetHeader *)g_AssetBase;
    if (loadedSize < (s32)sizeof(*header) ||
        header->audioHeaderOffset < (s32)sizeof(*header) ||
        header->sequenceOffset <= header->audioHeaderOffset ||
        header->audioBodyOffset <= header->sequenceOffset ||
        header->audioBodyOffset >= loadedSize) {
        FailAssetLoad();
        return;
    }
    g_AssetBlockPtr = g_AssetBase + header->audioHeaderOffset;
    g_AssetBlockSize =
        (size_t)(header->sequenceOffset - header->audioHeaderOffset);
    g_AssetBlockPtr2 = g_AssetBase + header->sequenceOffset;
    g_AssetBlock2Size =
        (size_t)(header->audioBodyOffset - header->sequenceOffset);
    g_AssetSubBlockPtr = g_AssetBase + header->audioBodyOffset;
    g_AssetSubBlockSize =
        (size_t)(loadedSize - header->audioBodyOffset);
    g_AssetLoadState = 0;
}

void LoadSelectBgmAssets(void) {
    switch (g_AssetLoadState) {
    case SELECT_BGM_CLOSE_AUDIO:
        CloseLoadedAudioSlots();
        g_AssetLoadState = SELECT_BGM_LOAD_ASSET;
        LoadSelectBgmAssetPack();
        break;
    case SELECT_BGM_LOAD_ASSET:
        LoadSelectBgmAssetPack();
        break;
    }
}
