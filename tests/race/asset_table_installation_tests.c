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
CourseObject *g_CourseObjects;
s32 g_CourseObjectCount;
char g_MsgEventOk[] = "";

static s32 s_failures;

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestTrackEventData(void) {
    TrackEventData data;
    u8 *offsetBase = (u8 *)&data.offsets;

    memset(&data, 0, sizeof(data));
    data.offsets.routeScenery = 32;
    data.offsets.raceIntroCamera = 64;
    data.offsets.pathSceneryPosition = 96;
    data.offsets.pathSceneryRotation = 128;
    data.offsets.flybyScenery = 160;

    InstallTrackEventData(&data);

    Check(g_TrackEventData == &data, "event data owner");
    Check((u8 *)g_RouteSceneryData == offsetBase + 32,
          "route scenery relocation");
    Check((u8 *)g_RaceIntroCameraScript == offsetBase + 64,
          "race intro camera relocation");
    Check((u8 *)g_PathSceneryPosData == offsetBase + 96,
          "path position relocation");
    Check((u8 *)g_PathSceneryRotData == offsetBase + 128,
          "path rotation relocation");
    Check((u8 *)g_FlybySceneryData == offsetBase + 160,
          "flyby scenery relocation");

    data.offsets.routeScenery = 1;
    InstallTrackEventData(&data);
    Check(g_RouteSceneryData == NULL, "misaligned event offset rejected");

    InstallTrackEventData(NULL);
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
