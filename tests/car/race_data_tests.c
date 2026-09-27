#include "game/race_data.h"
#include "game/car_catalog.h"
#include "game/asset_index.h"
#include "game/scene_asset.h"
#include "track_event_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static void Word(u8 *data, u32 value) {
    for (int i = 0; i < 4; i++) data[i] = (u8)(value >> (i * 8));
}
static void Entry(u8 *data, s32 index, u32 sector, u32 size) {
    Word(data + index * 8, sector);
    Word(data + index * 8 + 4, size);
}
static void TrackPack(u8 *data) {
    GameSceneAssetHeader *header = (GameSceneAssetHeader *)data;
    s32 cursor = sizeof(*header);
    for (int i = 0; i < SCENE_ASSET_BLOCK_COUNT; i++) {
        header->offsets[i] = cursor;
        cursor += i == 4 ? sizeof(s32) + 2 * sizeof(GameTrackPoint) :
                  i == 9 ? sizeof(TrackEventData) : 4;
    }
    TrackPointTable *points = (TrackPointTable *)(data + header->offsets[4]);
    points->count = 2;
    points->points[0].segmentLength = 100;
    points->points[1].segmentLength = 200;
    TrackEventData *events = (TrackEventData *)(data + header->offsets[9]);
    events->offsets = (TrackEventOffsets){.flybyScenery = 24, .routeScenery = 200,
        .raceIntroCamera = 424, .pathSceneryPosition = 608, .pathSceneryRotation = 692};
    SeedValidTrackEventTables(events);
}

int main(void) {
    const size_t carSize = sizeof(RaceCarAssetHeader) + sizeof(GameCarSpec) + 4;
    const size_t trackSize = sizeof(GameSceneAssetHeader) + 9 * 4 + sizeof(s32) +
        2 * sizeof(GameTrackPoint) + sizeof(TrackEventData);
    const u32 lastSector = 3 + (u32)((trackSize + 2047) / 2048);
    const u32 shapeSector = lastSector + (u32)((trackSize + 2047) / 2048);
    const size_t size = (shapeSector + 1) * 2048 + sizeof(CarShape) + 1;
    u8 *data = calloc(1, size);
    CHECK(data != NULL && carSize < 2048);
    for (int variant = 0; variant < CAR_MODEL_VARIANT_COUNT; variant++) {
        Entry(data, 11 + variant * 2, variant == 31 ? 2 : 1, (u32)carSize);
        Entry(data, 10 + variant * 2,
              variant == 31 ? shapeSector + 1 : shapeSector, sizeof(CarShape) + 1);
    }
    const CarShape firstShape = {100, -20, 300, 40};
    const CarShape lastShape = {150, -30, 350, 50};
    memcpy(data + shapeSector * 2048, &firstShape, sizeof(firstShape));
    memcpy(data + (shapeSector + 1) * 2048, &lastShape, sizeof(lastShape));
    data[shapeSector * 2048 + sizeof(CarShape)] = 1;
    RaceCarAssetHeader header = {sizeof(header), sizeof(header) + sizeof(GameCarSpec),
        sizeof(header) + sizeof(GameCarSpec) + 1, sizeof(header) + sizeof(GameCarSpec) + 2,
        sizeof(header) + sizeof(GameCarSpec) + 3};
    const GameCarSpec first = {.topGear = 6}, last = {.topGear = 2};
    memcpy(data + 2048, &header, sizeof header);
    memcpy(data + 2048 + header.specificationOffset, &first, sizeof first);
    memcpy(data + 4096, &header, sizeof header);
    memcpy(data + 4096 + header.specificationOffset, &last, sizeof last);
    TrackPack(data + 6144);
    TrackPack(data + lastSector * 2048);
    GameSceneAssetHeader *lastHeader = (GameSceneAssetHeader *)(data + lastSector * 2048);
    TrackPointTable *lastPoints = (TrackPointTable *)((u8 *)lastHeader + lastHeader->offsets[4]);
    lastPoints->points[0].segmentLength = 500;
    for (int index = 88; index <= 134; index += 2) {
        Entry(data, index, index == 134 ? lastSector : 3, (u32)trackSize);
    }
    RaceData archive, unchanged;
    CHECK(ReadRaceData(data, size, &archive));
    unchanged = archive;
    int automatic = -1;
    CHECK(ReadRaceCarTransmission(&archive, 0, &automatic) && automatic == 1);
    CHECK(ReadRaceCarTransmission(&archive, 31, &automatic) && automatic == 0);
    CHECK(!ReadRaceCarTransmission(&archive, -1, &automatic) && automatic == 0);
    CHECK(!ReadRaceCarTransmission(&archive, CAR_MODEL_VARIANT_COUNT, &automatic) && automatic == 0);
    CHECK(!ReadRaceCarTransmission(NULL, 0, &automatic) && automatic == 0);
    CHECK(!ReadRaceCarTransmission(&archive, 0, NULL));
    RaceData shortMetadata = archive;
    shortMetadata.entries[10].size = sizeof(CarShape);
    CHECK(!ReadRaceCarTransmission(&shortMetadata, 0, &automatic) && automatic == 0);
    size_t assetSize = 123;
    CHECK(RaceAsset(&archive, -1, &assetSize) == NULL && assetSize == 123);
    CHECK(RaceAsset(&archive, GAME_ASSET_COUNT, &assetSize) == NULL && assetSize == 123);
    CHECK(RaceAsset(NULL, 0, &assetSize) == NULL && assetSize == 123);
    CHECK(RaceAsset(&archive, 11, NULL) == NULL);
    CHECK(RaceAsset(&archive, 11, &assetSize) == data + 2048 && assetSize == carSize);
    const char *path = "race-archive-test.bin";
    FILE *file = fopen(path, "wb");
    CHECK(file != NULL && fwrite(data, 1, size, file) == size && fclose(file) == 0);
    RaceData *loaded = LoadRaceArchive(path), *another = LoadRaceArchive(path);
    CHECK(loaded != NULL && another != NULL && loaded->data != another->data);
    CarShape ownedShape;
    CHECK(ReadRaceCarShape(loaded, 31, &ownedShape));
    TrackData *ownedTrack = CopyRaceTrack(loaded, 5, 3);
    CHECK(ownedTrack != NULL);
    FreeRaceData(loaded);
    CHECK(memcmp(&ownedShape, &lastShape, sizeof(ownedShape)) == 0);
    CHECK(!ReadRaceCarShape(&archive, -1, &ownedShape));
    CHECK(!ReadRaceCarShape(&archive, CAR_MODEL_VARIANT_COUNT, &ownedShape));
    CHECK(!ReadRaceCarShape(NULL, 0, &ownedShape));
    CHECK(!ReadRaceCarShape(&archive, 0, NULL));
    CHECK(memcmp(&ownedShape, &lastShape, sizeof(ownedShape)) == 0);
    RaceData missingShape = archive;
    missingShape.entries[ASSET_CAR_1ST_BASE].size = 0;
    CHECK(!ReadRaceCarShape(&missingShape, 0, &ownedShape));
    missingShape.entries[ASSET_CAR_1ST_BASE].size = sizeof(CarShape) - 1;
    CHECK(!ReadRaceCarShape(&missingShape, 0, &ownedShape));
    CHECK(memcmp(&ownedShape, &lastShape, sizeof(ownedShape)) == 0);
    GameCarSpec ownedSpec;
    CHECK(ReadRaceCar(another, 31, &ownedSpec) && ownedSpec.topGear == 2);
    FreeRaceData(another);
    file = fopen(path, "wb");
    CHECK(file != NULL && fwrite(data, 1, size - 1, file) == size - 1 && fclose(file) == 0);
    CHECK(LoadRaceArchive(path) == NULL);
    CHECK(remove(path) == 0);
    CHECK(LoadRaceArchive(path) == NULL && LoadRaceArchive(NULL) == NULL);
    GameCarSpec spec;
    for (int variant = 0; variant < CAR_MODEL_VARIANT_COUNT; variant++) {
        CarShape shape;
        CHECK(ReadRaceCarShape(&archive, variant, &shape));
        CHECK(ReadRaceCarTransmission(&archive, variant, &automatic));
        CHECK(automatic == (variant != 31));
        CHECK(memcmp(&shape, variant == 31 ? &lastShape : &firstShape,
                     sizeof(shape)) == 0);
        CHECK(ReadRaceCar(&archive, variant, &spec));
        CHECK(spec.topGear == (variant == 31 ? 2 : 6));
    }
    /* Two room overlays must neither mutate the archive nor leak into another
     * room. Selection and application need no race/renderer initialization. */
    RageCarCatalog roomA, roomB;
    char error[128];
    CHECK(CarCatalogParse("[[cars]]\nid = \"last\"\nmodel = 12\ngrade = 0\nrev_limit = 9000\n",
                          &roomA, error, sizeof(error)));
    CHECK(CarCatalogParse("[[cars]]\nid = \"first\"\nmodel = 0\ngrade = 0\nrev_limit = 7000\n",
                          &roomB, error, sizeof(error)));
    GameCarSpec roomSpecA, roomSpecB, retailSpec;
    CHECK(ReadRaceCar(&archive, CarCatalogVariant(12, 0), &roomSpecA));
    retailSpec = roomSpecA;
    roomSpecB = roomSpecA;
    ApplyCarSpec(FindCarEntry(&roomA, 12, 0), &roomSpecA);
    ApplyCarSpec(FindCarEntry(&roomB, 12, 0), &roomSpecB);
    CHECK(roomSpecA.revLimit == 9000 && roomSpecA.topGear == 2);
    CHECK(memcmp(&roomSpecB, &retailSpec, sizeof(retailSpec)) == 0);
    GameCarSpec expected = retailSpec;
    expected.revLimit = 9000;
    CHECK(memcmp(&roomSpecA, &expected, sizeof(expected)) == 0);
    CHECK(ReadRaceCar(&archive, 31, &roomSpecB));
    CHECK(memcmp(&roomSpecB, &retailSpec, sizeof(retailSpec)) == 0);
    CHECK(memcmp(data + 4096 + header.specificationOffset, &last, sizeof(last)) == 0);
    CHECK(ReadRaceCar(&archive, 0, &roomSpecB));
    ApplyCarSpec(FindCarEntry(&roomB, 0, 0), &roomSpecB);
    CHECK(roomSpecB.revLimit == 7000 && roomSpecB.topGear == 6);
    CHECK(roomSpecA.revLimit == 9000);
    for (int cls = 0; cls < 6; cls++) {
        for (int course = 0; course < 4; course++) {
            TrackData *track = CopyRaceTrack(&archive, cls, course);
            CHECK(track != NULL && track->route.length == (cls == 5 && course == 3 ? 700 : 300));
            FreeTrackData(track);
        }
    }
    TrackData *retained = CopyRaceTrack(&archive, 5, 3);
    CHECK(retained != NULL);
    const GameCarSpec saved = spec;
    CHECK(!ReadRaceCar(&archive, -1, &spec) && !ReadRaceCar(&archive, 32, &spec));
    CHECK(!ReadRaceCar(NULL, 0, &spec) && !ReadRaceCar(&archive, 0, NULL));
    CHECK(memcmp(&spec, &saved, sizeof spec) == 0);
    CHECK(CopyRaceTrack(&archive, -1, 0) == NULL && CopyRaceTrack(&archive, 6, 0) == NULL);
    CHECK(CopyRaceTrack(&archive, 0, -1) == NULL && CopyRaceTrack(&archive, 0, 4) == NULL);
    CHECK(CopyRaceTrack(NULL, 0, 0) == NULL);
    for (size_t truncated = 0; truncated < 135 * 8; truncated++) {
        CHECK(!ReadRaceData(data, truncated, &archive));
        CHECK(memcmp(&archive, &unchanged, sizeof archive) == 0);
    }
    CHECK(!ReadRaceData(data, size - 1, &archive));
    CHECK(!ReadRaceData(NULL, size, &archive) && !ReadRaceData(data, size, NULL));
    Entry(data, 134, UINT32_MAX, 1);
    CHECK(!ReadRaceData(data, size, &archive));
    CHECK(memcmp(&archive, &unchanged, sizeof archive) == 0);
    Entry(data, 134, 0, 1);
    CHECK(!ReadRaceData(data, size, &archive));
    Entry(data, 134, 0, 0);
    CHECK(ReadRaceData(data, size, &archive));
    CHECK(CopyRaceTrack(&archive, 5, 3) == NULL);
    free(data);
    CHECK(spec.topGear == 2 && retained->route.length == 700);
    CHECK(ownedSpec.topGear == 2 && ownedTrack->route.length == 700);
    FreeTrackData(ownedTrack);
    FreeTrackData(retained);
    return 0;
}
