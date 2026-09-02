#include "common.h"
#include "game/race.h"
#include "game/track_internal.h"

#include <stdio.h>

s32 g_GrandPrixClass;

enum Event {
    UPDATE_ROUTE,
    UPDATE_FLYBY,
    UPDATE_PATH,
    DRAW_ROUTE,
    DRAW_FLYBY,
    DRAW_PATH,
};

static enum Event g_Events[6];
static s32 g_EventCount;

static void Record(enum Event event) {
    g_Events[g_EventCount++] = event;
}

void UpdateRouteScenery(void) { Record(UPDATE_ROUTE); }
void UpdateFlybyScenery(void) { Record(UPDATE_FLYBY); }
void UpdatePathScenery(void) { Record(UPDATE_PATH); }
void DrawRouteScenery(void) { Record(DRAW_ROUTE); }
void DrawFlybyScenery(void) { Record(DRAW_FLYBY); }
void DrawPathScenery(void) { Record(DRAW_PATH); }

static int Expect(s32 grandPrixClass, s32 animate,
                  const enum Event *expected, s32 expectedCount) {
    s32 index;

    g_GrandPrixClass = grandPrixClass;
    g_EventCount = 0;
    DrawScriptedScenery(animate);
    if (g_EventCount != expectedCount) {
        printf("FAIL: class %d animate %d produced %d events, expected %d\n",
               grandPrixClass, animate, g_EventCount, expectedCount);
        return 0;
    }
    for (index = 0; index < expectedCount; index++) {
        if (g_Events[index] != expected[index]) {
            printf("FAIL: class %d event %d was %d, expected %d\n",
                   grandPrixClass, index, g_Events[index], expected[index]);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    static const enum Event tier0Draw[] = {DRAW_ROUTE};
    static const enum Event tier0Animate[] = {UPDATE_ROUTE, DRAW_ROUTE};
    static const enum Event tier1Draw[] = {DRAW_ROUTE, DRAW_FLYBY};
    static const enum Event tier1Animate[] = {
        UPDATE_ROUTE, UPDATE_FLYBY, DRAW_ROUTE, DRAW_FLYBY,
    };
    static const enum Event tier3Draw[] = {
        DRAW_ROUTE, DRAW_FLYBY, DRAW_PATH,
    };
    static const enum Event tier3Animate[] = {
        UPDATE_ROUTE, UPDATE_FLYBY, UPDATE_PATH,
        DRAW_ROUTE, DRAW_FLYBY, DRAW_PATH,
    };

    if (!Expect(0, 0, tier0Draw, 1) ||
        !Expect(0, 1, tier0Animate, 2) ||
        !Expect(1, 0, tier1Draw, 2) ||
        !Expect(2, 1, tier1Animate, 4) ||
        !Expect(3, 0, tier3Draw, 3) ||
        !Expect(4, 1, tier3Animate, 6) ||
        !Expect(5, 1, tier0Animate, 2) ||
        !Expect(-1, 1, NULL, 0)) {
        return 1;
    }

    puts("scripted scenery dispatch preserved");
    return 0;
}
