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
void LoadBootAssets(void) {
    s32 loadedSize;
    u8 *base;

    switch (g_AssetLoadState) {
    case 1:
        base = GetAssetBytes(g_LoadBuffer);
        loadedSize = LoadAsset(1, base);
        if (loadedSize != 0) {
            UploadLoadBufferImage();
            g_AssetBlockPtr = base + loadedSize;
            g_AssetLoadState = 2;
        }
        break;
    case 2:
        loadedSize = LoadAsset(2, g_AssetBlockPtr);
        if (loadedSize != 0) {
            g_AssetLoadState = 3;
            g_AssetLoadCursor = g_AssetBlockPtr + loadedSize;
        }
        break;
    case 3:
        if (LoadAsset(3, g_AssetLoadCursor) != 0) {
            StartAudioSlotLoad(0, g_AssetBlockPtr, g_AssetLoadCursor, 0);
            g_AssetLoadState = 4;
        }
        break;
    case 4:
        if ((s16)PollAudioSlotLoad() != 0) {
            g_AssetLoadState = 5;
        }
        break;
    case 5:
        loadedSize = LoadAsset(4, g_AssetLoadCursor);
        if (loadedSize != 0) {
            printf("%s", g_MsgResOk);
            g_AssetLoadState = 6;
            g_AssetLoadCursor += loadedSize;
        }
        break;
    case 6:
        if (LoadAsset(5, g_AssetLoadCursor) != 0) {
            u8 *assetBase;

            UploadImageAsset(GetImageAssetHeaderWords(g_AssetLoadCursor));
            StoreImage(&g_TeamLogoClutRect, g_TeamLogoClut);
            StoreImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
            DrawSync(0);
            assetBase = g_AssetLoadCursor;
            g_TeamLogoClut[0] = 0;
            g_AssetLoadState = 0;
            g_AssetBase = assetBase;
        }
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
        if (LoadAsset(6, g_AssetBase) != 0) {
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
        if (LoadAsset(7, g_AssetBase) != 0) {
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
