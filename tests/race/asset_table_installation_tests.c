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
u8 g_MsgEventOk[] = "";

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
}

static void TestSimpleTables(void) {
    TrackRenderTable renderTable;
    struct {
        u32 count;
        CourseObject objects[2];
    } courseObjects;

    SetTrackRenderTable(&renderTable);
    courseObjects.count = 2;
    SetCourseObjects((CourseObjectTable *)&courseObjects);

    Check(g_TrackRenderTable == &renderTable, "track render table owner");
    Check(g_CourseObjects == courseObjects.objects, "course object table");
    Check(g_CourseObjectCount == 2, "course object count");
}

int main(void) {
    TestTrackEventData();
    TestSimpleTables();

    if (s_failures != 0) {
        return 1;
    }
    puts("asset table installers retain and relocate typed data");
    return 0;
}
