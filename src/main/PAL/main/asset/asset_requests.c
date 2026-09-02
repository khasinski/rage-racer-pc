#include <stdio.h>

#include "game/menu_internal.h"
#include "game/asset.h"
#include "game/cd.h"
#include "game/audio.h"
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
    StartAudioSlotLoad(0, g_AssetBlockPtr, g_AssetLoadCursor, NULL);
    g_AssetLoadState = 4;
}

static void WaitForBootAudio(void) {
    if ((s16)PollAudioSlotLoad() != 0) {
        g_AssetLoadState = 5;
    }
}

static void LoadBootResources(void) {
    s32 loadedSize = LoadAsset(ASSET_BOOT_RESOURCES, g_AssetLoadCursor);

    if (loadedSize == 0) return;
    printf("%s", g_MsgResOk);
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

static s32 RequestAsset(AssetRequestType request, s32 firstLoadState) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == request) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    ResetCdAudioState();
    g_AssetRequestType = request;
    g_AssetLoadState = firstLoadState;
    return 1;
}

s32 RequestSaveScreenAssets(void) {
    return RequestAsset(ASSET_REQUEST_SAVE_SCREEN, 1);
}

void LoadSaveScreenAssets(void) {
    if (g_AssetLoadState == 1) {
        if (LoadAsset(ASSET_SAVE_SCREEN, g_AssetBase) != 0) {
            g_AssetLoadState = 0;
            g_ImageBlockBuffer = g_AssetBase;
        }
    }
}

s32 RequestSelectBgmAssetsNoReset(void) {
    return RequestAsset(ASSET_REQUEST_SELECT_BGM, 2);
}

s32 RequestSelectBgmAssets(void) {
    return RequestAsset(ASSET_REQUEST_SELECT_BGM, 1);
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
            header = (GameSceneAssetHeader *)g_AssetBase;
            g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[0]);
            g_AssetBlockPtr2 = GetSceneAssetAddress(header, header->offsets[1]);
            g_AssetSubBlockPtr =
                GetSceneAssetAddress(header, header->offsets[2]);
            g_AssetLoadState = 0;
        }
        break;
    }
}
