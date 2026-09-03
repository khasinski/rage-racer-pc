#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

TrackEventData *g_TrackEventData;
SceneryMotionData *g_FlybySceneryData;
RaceIntroCameraScript *g_RaceIntroCameraScript;
SceneryMotionData *g_RouteSceneryData;
PathSceneryPositionData *g_PathSceneryPosData;
PathSceneryRotationData *g_PathSceneryRotData;
TrackRenderTable *g_TrackRenderTable;
const CourseObject *g_CourseObjects;
s32 g_CourseObjectCount;
static s32 s_failures;

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void SeedValidTrackEventTables(TrackEventData *data) {
    u8 *base = (u8 *)&data->offsets;
    SceneryMotionData *flyby =
        (SceneryMotionData *)(base + data->offsets.flybyScenery);
    SceneryMotionData *route =
        (SceneryMotionData *)(base + data->offsets.routeScenery);
    RaceIntroCameraScript *camera =
        (RaceIntroCameraScript *)(base + data->offsets.raceIntroCamera);
    PathSceneryPositionData *position =
        (PathSceneryPositionData *)(base + data->offsets.pathSceneryPosition);
    PathSceneryRotationData *rotation =
        (PathSceneryRotationData *)(base + data->offsets.pathSceneryRotation);

    flyby->keyframes[0].duration = 1;
    flyby->keyframes[1].duration = SCENERY_MOTION_END;
    route->keyframes[0].duration = 1;
    route->keyframes[1].duration = SCENERY_MOTION_END;
    camera->keys[0].mode = 1;
    camera->keys[0].duration = 0;
    position->keys[0].fields.span = 0;
    position->keys[1].fields.span = -1;
    position->keys[1].fields.loopIndex = 0;
    rotation->keys[0].fields.span = 0;
    rotation->keys[1].fields.span = -1;
    rotation->keys[1].fields.loopIndex = 0;
}

static void TestTrackEventData(void) {
    TrackEventData data;
    u8 *offsetBase = (u8 *)&data.offsets;

    memset(&data, 0, sizeof(data));
    data.offsets.flybyScenery = 24;
    data.offsets.routeScenery = 200;
    data.offsets.raceIntroCamera = 424;
    data.offsets.pathSceneryPosition = 608;
    data.offsets.pathSceneryRotation = 692;
    SeedValidTrackEventTables(&data);

    Check(InstallTrackEventData(&data, sizeof(data)) == 1,
          "valid event data accepted");

    Check(g_TrackEventData == &data, "event data owner");
    Check((u8 *)g_RouteSceneryData == offsetBase + 200,
          "route scenery relocation");
    Check((u8 *)g_RaceIntroCameraScript == offsetBase + 424,
          "race intro camera relocation");
    Check((u8 *)g_PathSceneryPosData == offsetBase + 608,
          "path position relocation");
    Check((u8 *)g_PathSceneryRotData == offsetBase + 692,
          "path rotation relocation");
    Check((u8 *)g_FlybySceneryData == offsetBase + 24,
          "flyby scenery relocation");

    data.offsets.routeScenery = data.offsets.flybyScenery;
    Check(InstallTrackEventData(&data, sizeof(data)) == 0,
          "overlapping event blocks rejected");
    data.offsets.routeScenery = 200;

    ((SceneryMotionData *)(offsetBase + data.offsets.routeScenery))
        ->firstKeyframe[0][0] = INT16_MAX;
    Check(InstallTrackEventData(&data, sizeof(data)) == 0,
          "event key index beyond block rejected");
    ((SceneryMotionData *)(offsetBase + data.offsets.routeScenery))
        ->firstKeyframe[0][0] = 0;

    ((PathSceneryPositionData *)(offsetBase +
        data.offsets.pathSceneryPosition))->keys[1].fields.span = 1;
    Check(InstallTrackEventData(&data, sizeof(data)) == 0,
          "unterminated path table rejected");
    ((PathSceneryPositionData *)(offsetBase +
        data.offsets.pathSceneryPosition))->keys[1].fields.span = -1;

    ((RaceIntroCameraScript *)(offsetBase +
        data.offsets.raceIntroCamera))->keys[0].mode = 2;
    Check(InstallTrackEventData(&data, sizeof(data)) == 0,
          "unknown intro camera mode rejected");
    ((RaceIntroCameraScript *)(offsetBase +
        data.offsets.raceIntroCamera))->keys[0].mode = 1;

    data.offsets.routeScenery = sizeof(data.offsets) - sizeof(s32);
    Check(InstallTrackEventData(&data, sizeof(data)) == 0,
          "event header offset rejected");
    Check(g_TrackEventData == NULL && g_RouteSceneryData == NULL,
          "invalid event data clears published views");

    data.offsets.routeScenery = sizeof(data.offsets) + 1;
    InstallTrackEventData(&data, sizeof(data));
    Check(g_RouteSceneryData == NULL, "misaligned event offset rejected");

    data.offsets.routeScenery = (s32)(sizeof(data) -
                                      offsetof(TrackEventData, offsets));
    InstallTrackEventData(&data, sizeof(data));
    Check(g_TrackEventData == NULL && g_RouteSceneryData == NULL,
          "event offset beyond block rejected");

    InstallTrackEventData(NULL, 0);
    Check(g_TrackEventData == NULL && g_RouteSceneryData == NULL &&
              g_RaceIntroCameraScript == NULL &&
              g_PathSceneryPosData == NULL && g_PathSceneryRotData == NULL &&
              g_FlybySceneryData == NULL,
          "null event table clears published views");
}

int main(void) {
    TestTrackEventData();

    if (s_failures != 0) {
        return 1;
    }
    puts("asset table installers retain and relocate typed data");
    return 0;
}
