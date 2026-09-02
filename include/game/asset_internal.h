#ifndef GAME_ASSET_INTERNAL_H
#define GAME_ASSET_INTERNAL_H

#include "common.h"
#include "game/asset.h"
#include "game/menu_types.h"

extern TeamLogoSample *g_TeamLogoSampleData;
extern char g_TextNowLoading[];
extern s32 g_AssetLoadFailed;

static inline void FailAssetLoad(void) {
    g_AssetLoadFailed = 1;
    g_AssetLoadState = 0;
}

s32 RequestAssetLoad(AssetRequestType request, s32 firstLoadState,
                     s32 resetCdAudio);
s32 IsValidModelBankAsset(const ModelBankHeader *base, size_t size);
s32 IsValidCourseModelAsset(const CourseModelAssetHeader *base, size_t size);
s32 IsValidTerrainCellAsset(const void *data, size_t size);
s32 IsValidSerializedCarModelAsset(const CarModelAsset *asset, size_t size);
s32 InstallSerializedCarModelSlot(CarModelAsset *asset, s32 index);
CarModelAsset *FindSerializedCarModelAsset(CarModelAsset *nativeAsset);
s32 IsValidImageAsset(const GameImageAssetHeaderWord *asset, size_t size);
s32 IsValidImageEntry(const GameImageEntryHeader *entry, size_t size);

#endif
