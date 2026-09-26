#include "track_event_fixture.h"
#include "game/track_data.h"
#include "game/scene_asset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const size_t pointSize = sizeof(s32) + 2 * sizeof(GameTrackPoint);
    const size_t size = sizeof(GameSceneAssetHeader) + 9 * 4 + pointSize + sizeof(TrackEventData);
    u8 *pack = calloc(1, size);
    CHECK(pack != NULL);
    GameSceneAssetHeader *header = (GameSceneAssetHeader *)pack;
    size_t cursor = sizeof(*header);
    for (int i = 0; i < SCENE_ASSET_BLOCK_COUNT; i++) {
        header->offsets[i] = (s32)cursor;
        cursor += i == 4 ? pointSize : i == 9 ? sizeof(TrackEventData) : 4;
    }
    TrackPointTable *points = (TrackPointTable *)(pack + header->offsets[4]);
    points->count = 2;
    points->points[0].segmentLength = 100;
    points->points[1].segmentLength = 200;
    TrackEventData *events = (TrackEventData *)(pack + header->offsets[9]);
    events->offsets = (TrackEventOffsets){.flybyScenery = 24, .routeScenery = 200,
        .raceIntroCamera = 424, .pathSceneryPosition = 608, .pathSceneryRotation = 692};
    SeedValidTrackEventTables(events);
    SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT];
    CHECK(ReadSceneAssetBlocks(pack, size, blocks));
    TrackData borrowed;
    CHECK(ReadTrackData(blocks, &borrowed));
    CHECK(borrowed.route.points == points->points && borrowed.events == events);
    CHECK(borrowed.route.length == 300);
    const TrackData previous = borrowed;
    CHECK(!ReadTrackData(NULL, &borrowed));
    CHECK(!ReadTrackData(blocks, NULL));
    const size_t eventSize = blocks[9].size;
    blocks[9].size = 0; /* Valid route followed by malformed events. */
    CHECK(!ReadTrackData(blocks, &borrowed));
    CHECK(memcmp(&borrowed, &previous, sizeof(borrowed)) == 0);
    blocks[9].size = eventSize;
    CHECK(ReadTrackData(blocks, &borrowed));
    TrackData *first = CopyTrackData(pack, size);
    CHECK(first != NULL && first->route.length == 300);
    CHECK(first->route.points != points->points && first->events != events);
    const char *path = "track-data-test.bin";
    FILE *file = fopen(path, "wb");
    CHECK(file != NULL && fwrite(pack, 1, size, file) == size);
    CHECK(fclose(file) == 0);
    TrackData *loaded = LoadTrackData(path);
    CHECK(remove(path) == 0);
    CHECK(loaded != NULL && loaded->route.length == 300);
    points->points[0].segmentLength = 500;
    events->trackWalkStart = 17;
    TrackData *second = CopyTrackData(pack, size);
    CHECK(second != NULL && second->route.length == 700 && second->events->trackWalkStart == 17);
    CHECK(first->route.length == 300 && first->events->trackWalkStart == 0);
    CHECK(CopyTrackData(pack, sizeof(*header) - 1) == NULL);
    header->offsets[5] = header->offsets[4];
    CHECK(CopyTrackData(pack, size) == NULL);
    free(pack);
    CHECK(first->route.points[0].segmentLength == 100 && second->route.points[0].segmentLength == 500);
    CHECK(LoadTrackData(path) == NULL && LoadTrackData(NULL) == NULL);
    FreeTrackData(first); FreeTrackData(second); FreeTrackData(loaded); FreeTrackData(NULL);
    return 0;
}
