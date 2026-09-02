#ifndef GAME_ASSET_INTERNAL_H
#define GAME_ASSET_INTERNAL_H

#include "common.h"
#include "game/asset.h"
#include "game/menu_types.h"

extern TeamLogoSample *g_TeamLogoSampleData;
extern char g_TextNowLoading[];

s32 RequestAssetLoad(AssetRequestType request, s32 firstLoadState,
                     s32 resetCdAudio);
s32 IsValidModelBankAsset(const ModelBankHeader *base, size_t size);
s32 IsValidCourseModelAsset(const CourseModelAssetHeader *base, size_t size);
s32 IsValidTerrainCellAsset(const void *data, size_t size);

#endif
