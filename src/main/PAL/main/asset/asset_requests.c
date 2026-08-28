#include "game/menu_internal.h"
#include "game/asset.h"
#include "game/cd.h"
#include "game/audio.h"

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
            InstallResourceData(g_AssetLoadCursor);
            g_AssetLoadState = 6;
            g_AssetLoadCursor += loadedSize;
        }
        break;
    case 6:
        if (LoadAsset(5, g_AssetLoadCursor) != 0) {
            u8 *assetBase;

            UploadImageAsset(g_AssetLoadCursor);
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

s32 RequestSaveScreenAssets(void) {
    s32 state;

    if (g_AssetLoadState != 0) {
        return 1;
    }

    state = ASSET_REQUEST_SAVE_SCREEN;
    if (g_AssetRequestType == state) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    ResetCdAudioState();
    g_AssetRequestType = state;
    g_AssetLoadState = 1;
    return 1;
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
    s32 loadType;

    if (g_AssetLoadState != 0) {
        return 1;
    }

    loadType = ASSET_REQUEST_SELECT_BGM;
    if (g_AssetRequestType == loadType) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    ResetCdAudioState();
    g_AssetRequestType = loadType;
    g_AssetLoadState = 2;
    return 1;
}

s32 RequestSelectBgmAssets(void) {
    s32 loadType;

    if (g_AssetLoadState != 0) {
        return 1;
    }

    loadType = ASSET_REQUEST_SELECT_BGM;
    if (g_AssetRequestType == loadType) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    ResetCdAudioState();
    g_AssetRequestType = loadType;
    g_AssetLoadState = 1;
    return 1;
}

void LoadSelectBgmAssets(void) {
    GameSceneAssetHeader *header;
    s32 firstOffset;
    u8 *secondBlock;
    s32 thirdOffset;
    s32 relOffset;
    GameSceneAssetAddress firstBlockAddress;
    GameSceneAssetOffsetAddress offsetAddress;

    switch (g_AssetLoadState) {
    case 1:
        CloseLoadedAudioSlots();
        g_AssetLoadState = 2;
        /* fall through */
    case 2:
        if (LoadAsset(7, g_AssetBase) != 0) {
            firstBlockAddress.pointer = g_AssetBase;
            header = firstBlockAddress.header;
            /* The three volatile reads are load-bearing: without them cse
             * folds the header pointer and the three offsets into one
             * addressing pattern and the block costs 19 instructions more. */
            offsetAddress.pointer = &header->offsets[0];
            firstOffset = *offsetAddress.volatilePointer;
            offsetAddress.pointer = &header->offsets[2];
            thirdOffset = *offsetAddress.volatilePointer;
            firstBlockAddress.header = header;
            firstBlockAddress.bytes = GetAssetBytes(header) + firstOffset;
            g_AssetBlockPtr = firstBlockAddress.pointer;
            offsetAddress.pointer = &header->offsets[1];
            relOffset = *offsetAddress.volatilePointer;
            g_AssetLoadState = 0;
            secondBlock = GetSceneAssetAddress(header, relOffset);
            header = (GameSceneAssetHeader *)(GetAssetBytes(header) + thirdOffset);
            g_AssetBlockPtr2 = secondBlock;
            g_AssetSubBlockPtr = GetAssetBytes(header);
        }
        break;
    }
}
