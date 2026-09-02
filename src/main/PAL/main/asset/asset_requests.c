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

static void LoadBootTitleScreen(void) {
    u8 *base = GetAssetBytes(g_LoadBuffer);
    s32 loadedSize = LoadAsset(ASSET_TITLE_SCREEN, base);

    if (loadedSize == 0) return;
    UploadLoadBufferImage();
    g_AssetBlockPtr = base + loadedSize;
    g_AssetLoadState = BOOT_LOAD_AUDIO_HEADER;
}

static void LoadBootAudioHeader(void) {
    s32 loadedSize = LoadAsset(ASSET_BOOT_AUDIO_HEADER, g_AssetBlockPtr);

    if (loadedSize == 0) return;
    g_AssetLoadCursor = g_AssetBlockPtr + loadedSize;
    g_AssetLoadState = BOOT_LOAD_AUDIO_BODY;
}

static void LoadBootAudioBody(void) {
    if (LoadAsset(ASSET_BOOT_AUDIO_BODY, g_AssetLoadCursor) == 0) return;
    StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, g_AssetBlockPtr,
                       g_AssetLoadCursor, NULL);
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

    if (LoadAsset(ASSET_BOOT_CAR_SCREEN, assetBase) == 0) return;
    UploadImageAsset(GetImageAssetHeaderWords(assetBase));
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
    if (g_AssetLoadState == SAVE_SCREEN_LOAD_ASSET) {
        if (LoadAsset(ASSET_SAVE_SCREEN, g_AssetBase) != 0) {
            g_AssetLoadState = 0;
            g_ImageBlockBuffer = g_AssetBase;
        }
    }
}

s32 RequestSelectBgmAssetsKeepAudioSlots(void) {
    return RequestAssetLoad(ASSET_REQUEST_SELECT_BGM,
                            SELECT_BGM_LOAD_ASSET, 1);
}

s32 RequestSelectBgmAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_SELECT_BGM,
                            SELECT_BGM_CLOSE_AUDIO, 1);
}

void LoadSelectBgmAssets(void) {
    GameSceneAssetHeader *header;

    switch (g_AssetLoadState) {
    case SELECT_BGM_CLOSE_AUDIO:
        CloseLoadedAudioSlots();
        g_AssetLoadState = SELECT_BGM_LOAD_ASSET;
        /* fall through */
    case SELECT_BGM_LOAD_ASSET:
        if (LoadAsset(ASSET_SELECT_BGM, g_AssetBase) != 0) {
            header = GetSceneAssetHeader(g_AssetBase);
            g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[0]);
            g_AssetBlockPtr2 = GetSceneAssetAddress(header, header->offsets[1]);
            g_AssetSubBlockPtr =
                GetSceneAssetAddress(header, header->offsets[2]);
            g_AssetLoadState = 0;
        }
        break;
    }
}
