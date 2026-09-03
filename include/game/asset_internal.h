#ifndef GAME_ASSET_INTERNAL_H
#define GAME_ASSET_INTERNAL_H

#include "common.h"
#include "game/asset.h"
#include "game/menu_types.h"

extern const TeamLogoSample *g_TeamLogoSampleData;
extern char g_TextNowLoading[];
extern s32 g_AssetLoadFailed;

static inline s32 AssetPayloadOffsetIsValid(s32 offset,
                                            size_t payloadOffset,
                                            size_t size) {
    return offset >= 0 && offset % (s32)sizeof(s32) == 0 &&
           (size_t)offset >= payloadOffset && (size_t)offset < size;
}

/* Writable bytes remaining in the port-owned buffer containing `at`. */
size_t PortAssetRoomAt(const void *at);

/* Computes a byte span without relational comparison or subtraction between
 * C pointers that may have lost their common-array provenance in host state. */
static inline s32 AssetSpanSize(const void *begin, const void *end,
                                size_t *size) {
    uintptr_t first;
    uintptr_t last;

    if (begin == NULL || end == NULL || size == NULL) {
        return 0;
    }
    first = (uintptr_t)begin;
    last = (uintptr_t)end;
    if (last <= first || last - first > SIZE_MAX) {
        return 0;
    }
    *size = (size_t)(last - first);
    return 1;
}

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
s32 IsValidEnvironmentScript(const struct GameEnvironmentScript *script,
                             size_t size);
s32 IsValidTrackPointAsset(const struct TrackPointTable *trackData,
                           size_t size);
s32 IsValidTrackEventAsset(const struct TrackEventData *eventData,
                           size_t size);
s32 InstallSerializedCarModelSlot(CarModelAsset *asset, size_t size,
                                  s32 index);
CarModelAsset *FindSerializedCarModelAsset(CarModelAsset *nativeAsset);
s32 IsValidImageAsset(const GameImageAssetHeaderWord *asset, size_t size);
s32 IsValidImageEntry(const GameImageEntryHeader *entry, size_t size);

#endif
