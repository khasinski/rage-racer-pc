#include "game/race_data.h"
#include "game/asset_index.h"

_Static_assert(GAME_ASSET_COUNT == RAGE_ARCHIVE_INDEX_ENTRY_COUNT,
               "game and standalone must share the archive layout");

int ReadRaceData(const void *data, size_t size, RaceData *archive) {
    RaceData view = {.data = data, .size = size};
    if (archive == NULL || !RageArchiveDecodeIndex(data, size, view.entries, GAME_ASSET_COUNT)) return 0;
    for (s32 i = 0; i < GAME_ASSET_COUNT; i++) {
        const RageArchiveIndexEntry entry = view.entries[i];
        if (entry.size == 0) continue;
        if (entry.byteOffset < GAME_ASSET_COUNT * RAGE_ARCHIVE_INDEX_ENTRY_SIZE ||
            entry.byteOffset > size || entry.size > size - entry.byteOffset) return 0;
    }
    *archive = view;
    return 1;
}

const void *RaceAsset(const RaceData *archive, s32 index, size_t *size) {
    if (size == NULL || archive == NULL || archive->data == NULL || (u32)index >= GAME_ASSET_COUNT) return NULL;
    const RageArchiveIndexEntry entry = archive->entries[index];
    if (entry.size == 0 || entry.byteOffset > archive->size ||
        entry.size > archive->size - entry.byteOffset) return NULL;
    *size = entry.size;
    return archive->data + entry.byteOffset;
}

int ReadRaceCar(const RaceData *archive, s32 variant, GameCarSpec *spec) {
    if ((u32)variant >= CAR_MODEL_VARIANT_COUNT) return 0;
    size_t size = 0;
    const void *data = RaceAsset(archive, CarVariantAssetIndex(ASSET_CAR_2ND_BASE, variant), &size);
    return ReadCarSpec(data, size, spec);
}

int ReadRaceCarShape(const RaceData *archive, s32 variant, CarShape *shape) {
    if ((u32)variant >= CAR_MODEL_VARIANT_COUNT) return 0;
    size_t size = 0;
    const void *data = RaceAsset(archive,
        CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variant), &size);
    return ReadCarShape(data, size, shape);
}

CarModelData *CopyRaceCarModel(const RaceData *archive, s32 variant) {
    if ((u32)variant >= CAR_MODEL_VARIANT_COUNT) return NULL;
    size_t size = 0;
    const void *data = RaceAsset(archive,
        CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variant), &size);
CarModelData *model = CopyCarModelData(data, size);
if (!model) return NULL;
const void *shared = RaceAsset(archive, ASSET_BOOT_CAR_SCREEN, &size);
if (!ReadCarModelImages(model, shared, size)) {
    FreeCarModelData(model);
    return NULL;
}
return model;
}

TrackData *CopyRaceTrack(const RaceData *archive, s32 classIndex, s32 courseIndex) {
    size_t size = 0;
    const void *data = RaceAsset(archive, TrackCourseAssetIndex(ASSET_TRACK_2ND_BASE,
                                                          classIndex, courseIndex), &size);
    return CopyTrackData(data, size);
}

SceneAsset *CopyRaceScene(const RaceData *archive, s32 classIndex, s32 courseIndex) {
    size_t size = 0;
    const void *data = RaceAsset(archive, TrackCourseAssetIndex(ASSET_TRACK_2ND_BASE,
                                                          classIndex, courseIndex), &size);
    return CopySceneAsset(data, size);
}

TrackImages *CopyRaceImages(const RaceData *archive, s32 classIndex, s32 courseIndex) {
    size_t size = 0;
    const void *data = RaceAsset(archive, TrackCourseAssetIndex(ASSET_TRACK_1ST_BASE,
                                                          classIndex, courseIndex), &size);
    return CopyTrackImages(data, size);
}
