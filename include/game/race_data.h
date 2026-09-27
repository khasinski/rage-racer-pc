#ifndef GAME_RACE_DATA_H
#define GAME_RACE_DATA_H
#include "game/archive_index.h"
#include "game/car_asset.h"
#include "game/car_model_data.h"
#include "game/track_data.h"
#include "game/scene_asset.h"
#include "game/track_images.h"

/* Immutable archive view. The source buffer belongs to the caller and must
 * outlive the view. Loaded specifications and copied tracks are independent. */
typedef struct RaceData {
    const u8 *data;
    size_t size;
    RageArchiveIndexEntry entries[RAGE_ARCHIVE_INDEX_ENTRY_COUNT];
    /* Disc boot serial when identified; empty for a bare archive or an
     * unidentified disc. A serial alone does not distinguish all revisions. */
    char boot[16];
    /* Fingerprint of the disc executable; zero when unavailable/bare archive. */
    uint64_t executable;
} RaceData;

/* Validates the complete index and every nonempty range. Failure leaves the
 * output unchanged. No globals, renderer, mod configuration or disc calls. */
int ReadRaceData(const void *data, size_t size, RaceData *archive);
/* Borrows one bounded raw asset; size is written only on success. */
const void *RaceAsset(const RaceData *archive, s32 index, size_t *size);
int ReadRaceCar(const RaceData *archive, s32 variant, GameCarSpec *spec);
int ReadRaceCarShape(const RaceData *archive, s32 variant, CarShape *shape);
CarModelData *CopyRaceCarModel(const RaceData *archive, s32 variant);
/* classIndex is 0..5, courseIndex is the physical course slot 0..3.
 * Reverse uses the same pack; the race selects its reverse event tables. */
/* Owned runtime pack; typed render validation is the consumer's responsibility. */
TrackImages *CopyRaceImages(const RaceData *archive, s32 classIndex, s32 courseIndex);
SceneAsset *CopyRaceScene(const RaceData *archive, s32 classIndex, s32 courseIndex);
TrackData *CopyRaceTrack(const RaceData *archive, s32 classIndex, s32 courseIndex);
/* Owned archives loaded without game state. Disc accepts CUE or raw Track 01
 * BIN; archive accepts an extracted RAGE.BIN. Return NULL on any read/validation
 * failure. FreeRaceData is only for objects returned by these loaders, not
 * caller-owned views constructed with ReadRaceData. */
RaceData *LoadRaceDisc(const char *path);
RaceData *LoadRaceArchive(const char *path);
void FreeRaceData(RaceData *archive);
#endif
