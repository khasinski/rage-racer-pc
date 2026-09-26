#include "track_event_fixture.h"
#include "game/track.h"
#include <string.h>

int main(void) {
    TrackEventData first = {0};
    first.offsets.flybyScenery = 24;
    first.offsets.routeScenery = 200;
    first.offsets.raceIntroCamera = 424;
    first.offsets.pathSceneryPosition = 608;
    first.offsets.pathSceneryRotation = 692;
    SeedValidTrackEventTables(&first);
    TrackEventData second = first;
    second.trackWalkStart = 17;
    if (ReadTrackEvents(&first, sizeof(first)) != &first ||
        ReadTrackEvents(&second, sizeof(second)) != &second) return 1;
    TrackEventData baseline = first;
    for (size_t size = 0; size < sizeof(first); size++) {
        if (ReadTrackEvents(&first, size) != NULL) return 1;
    }
    if (memcmp(&first, &baseline, sizeof(first)) != 0) return 1;
    first.offsets.routeScenery = first.offsets.flybyScenery;
    if (ReadTrackEvents(&first, sizeof(first)) != NULL) return 1;
    first = baseline;
    u8 *base = (u8 *)&first.offsets;
    SceneryMotionData *motion = (SceneryMotionData *)(base + first.offsets.flybyScenery);
    motion->firstKeyframe[0][0] = INT16_MAX;
    if (ReadTrackEvents(&first, sizeof(first)) != NULL) return 1;
    first = baseline;
    PathSceneryPositionData *path = (PathSceneryPositionData *)(base + first.offsets.pathSceneryPosition);
    path->keys[1].loopIndex = 1;
    if (ReadTrackEvents(&first, sizeof(first)) != NULL) return 1;
    if (ReadTrackEvents(&second, sizeof(second)) != &second || ReadTrackEvents(NULL, 0) != NULL) return 1;
    return 0;
}
