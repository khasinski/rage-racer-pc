#include "common.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/state.h"
#include "game/race.h"
#include "game/render.h"
#include "game/cd.h"
#include "game/random.h"


/* Where asset 0x56 lands: g_ImageBlockBuffer advanced past the car texture
 * block just loaded. Its header words 1 and 2 are relocated into
 * g_AssetBlockPtr / g_AssetSubBlockPtr and word 0 is kept as-is. */

void LoadUpgradedCarModel(s32 carIndex) {
    u8 *ptr;
    CarModelAsset *asset;
    s32 offset;
    s32 assetId;
    u32 mode;

    if (g_AssetLoadState == 1) {
        offset = GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant + 1) << 1;
        mode = g_CarModelSlot;
        ptr = g_CarModelBuffer;
        assetId = offset + 0xA;

        if (mode == 0) {
            ptr += 0x20000;
        }

        if (LoadAsset(assetId, ptr) != 0) {
            asset = GetCarModelAsset(ptr);
            SetCarModelSlot(asset, g_CarModelSlot < 1);
            asset = g_CarModelSlots[g_CarModelSlot < 1];
            RegisterModelBank(asset->modelData.modelBank, g_CarModelSlot < 1);
            SetCarImageSlot(asset->imageData.carImage, g_CarModelSlot < 1);

            if (g_PlayerCarIndex < 10) {
                ApplyBodyColor1(g_CarTable[carIndex].paintColor1,
                                asset->imageData.carImage);
                ApplyBodyColor2(g_CarTable[carIndex].paintColor2,
                                asset->imageData.carImage);
            }

            g_AssetLoadState = 0;
        }
    }
}

s32 RequestOptionScreenAssets(void) {
    s32 state;

    if (g_AssetLoadState != 0) {
        return 1;
    }

    state = ASSET_REQUEST_OPTION_SCREEN;
    if (g_AssetRequestType == state) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    ResetCdAudioState();
    g_AssetRequestType = state;
    g_AssetLoadState = 1;
    return 1;
}

void LoadOptionScreenAssets(void) {
    u8 *base;
    s32 offset;

    if (g_AssetLoadState == 1) {
        if (LoadAsset(9, g_AssetBase) != 0) {
            RegisterModelBank(&GetOptionScreenAsset(g_AssetBase)->modelBank, 0);
            SelectModelBank(0);

            base = g_AssetBase;
            offset = GetOptionScreenAsset(base)->imageOffset;
            g_AssetLoadState = 0;
            g_ImageBlockBuffer = base + offset;
        }
    }
}

s32 RequestRoundAssets(void) {
    s32 value;

    if (g_AssetLoadState != 0) {
        ResetAssetLoader();
    }

    if (g_GrandPrixMode == 0) {
        value = (Random15() & 0xFFF) % (g_MaxClassReached[g_GrandPrixSeries] + 1);
        g_GrandPrixClass = value;
        if (((RageSeriesCourseIndex()) == 3) && (value < 2)) {
            g_GrandPrixClass = ((Random15() & 0xFFF) % (g_MaxClassReached[g_GrandPrixSeries] - 1)) + 2;
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

void LoadRoundAssets(void) {
    s32 state;
    s32 kind;
    s32 result;

    state = g_AssetLoadState;
    switch (state) {
    case 1:
        kind = ASSET_TIME_ATTACK_ROUND_SCREEN;
        if (g_GrandPrixMode != 0) {
            kind = AssetRoundScreenId(g_GrandPrixSeries, g_GrandPrixClass);
        }

        result = LoadAsset(kind, g_ImageBlockBuffer);
        if (result != 0) {
            g_AssetLoadState = 2;
            g_AssetBlockPtr2 = g_ImageBlockBuffer + result;
        }
        break;
    case 2:
        if (LoadAsset(ASSET_VOICE_BANK, g_AssetBlockPtr2) != 0) {
            u8 *header;
            u8 *first;
            u8 *second;
            s32 shared;

            header = g_AssetBlockPtr2;
            first = header + GetAssetWords(header)[1];
            second = header + GetAssetWords(header)[2];
            g_AssetBlockPtr = first;
            g_AssetSubBlockPtr = second;
            shared = *GetAssetWords(header);
            g_AssetLoadState = 0;
            g_SharedAssetWord0 = shared;
        }
        break;
    }
}

void RelocateCarModel(void) {
    u32 *dst;
    u32 *src;
    u32 byteCount;
    u32 wordCount;

    src = (u32 *)GetSerializedCarModelAsset(g_CarModelAsset);
    dst = (u32 *)g_AssetBase;
    byteCount = src[6] + 0x28;
    wordCount = byteCount >> 2;
    g_AssetLoadCursor = g_AssetBase + byteCount;

    while (wordCount != 0) {
        *dst = *src;
        src++;
        wordCount--;
        dst++;
    }

    SetCarModelSlot(GetCarModelAsset(g_AssetBase), 0);
    SelectCarModelSlot(0);
    UnrelocateModelBank(GetModelBankHeader(g_AssetBase + 0x28), 0);
    g_CarModelAsset->modelData.pointer = g_AssetBase + 0x28;
    RegisterModelBank(GetModelBankHeader(g_AssetBase + 0x28), 0);
}
