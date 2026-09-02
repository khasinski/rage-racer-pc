#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/cd.h"
#include "game/audio.h"
#include "game/menu_internal.h"
#include "game/race.h"

/* The team-logo canvas and its two VRAM rects. Kept local: menu/ spells the
 * canvas u32[] for the nibble transforms and u8[] where it only wants the
 * address, and draw_team_logo_canvas.c reads the two rects through its own
 * TeamLogoClutPos / TeamLogoTexturePos structs, so there is no single type
 * to hoist without rewriting those indices. */
static void LoadBootTitleScreen(void) {
    u8 *base = GetAssetBytes(g_LoadBuffer);
    s32 loadedSize = LoadAsset(ASSET_TITLE_SCREEN, base);

    if (loadedSize == 0) return;
    UploadLoadBufferImage();
    g_AssetBlockPtr = base + loadedSize;
    g_AssetLoadState = 2;
}

static void LoadBootAudioHeader(void) {
    s32 loadedSize = LoadAsset(ASSET_BOOT_AUDIO_HEADER, g_AssetBlockPtr);

    if (loadedSize == 0) return;
    g_AssetLoadCursor = g_AssetBlockPtr + loadedSize;
    g_AssetLoadState = 3;
}

static void LoadBootAudioBody(void) {
    if (LoadAsset(ASSET_BOOT_AUDIO_BODY, g_AssetLoadCursor) == 0) return;
    StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, g_AssetBlockPtr,
                       g_AssetLoadCursor, NULL);
    g_AssetLoadState = 4;
}

static void WaitForBootAudio(void) {
    if (PollAudioSlotLoad() != 0) {
        g_AssetLoadState = 5;
    }
}

static void LoadBootResources(void) {
    s32 loadedSize = LoadAsset(ASSET_BOOT_RESOURCES, g_AssetLoadCursor);

    if (loadedSize == 0) return;
    g_AssetLoadCursor += loadedSize;
    g_AssetLoadState = 6;
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
    case 1:
        LoadBootTitleScreen();
        break;
    case 2:
        LoadBootAudioHeader();
        break;
    case 3:
        LoadBootAudioBody();
        break;
    case 4:
        WaitForBootAudio();
        break;
    case 5:
        LoadBootResources();
        break;
    case 6:
        LoadBootCarScreen();
        break;
    }
}

s32 RequestSaveScreenAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_SAVE_SCREEN, 1, 1);
}

void LoadSaveScreenAssets(void) {
    if (g_AssetLoadState == 1) {
        if (LoadAsset(ASSET_SAVE_SCREEN, g_AssetBase) != 0) {
            g_AssetLoadState = 0;
            g_ImageBlockBuffer = g_AssetBase;
        }
    }
}

s32 RequestSelectBgmAssetsKeepAudioSlots(void) {
    return RequestAssetLoad(ASSET_REQUEST_SELECT_BGM, 2, 1);
}

s32 RequestSelectBgmAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_SELECT_BGM, 1, 1);
}

void LoadSelectBgmAssets(void) {
    GameSceneAssetHeader *header;

    switch (g_AssetLoadState) {
    case 1:
        CloseLoadedAudioSlots();
        g_AssetLoadState = 2;
        /* fall through */
    case 2:
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
