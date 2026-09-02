#include "common.h"
#include "game/race.h"

#include <stdio.h>

void DrawCourseScenery(s32 course, s32 timer, s32 animate);
void DrawCourseScenery2(s32 timer, s32 animate);

s32 g_CourseIndex;
s32 g_GrandPrixClass;
s32 g_SceneId;

typedef struct Event {
    s32 id;
    s32 a;
    s32 b;
    s32 c;
} Event;

enum {
    EVENT_ANIMATED = 1,
    EVENT_ANIMATED_2,
    EVENT_SPINNING,
    EVENT_HIGH_CLASS,
    EVENT_STATIC,
    EVENT_UPDATE_SHUTTLE,
    EVENT_DRAW_SHUTTLE,
};

static Event g_Events[16];
static s32 g_EventCount;

static void Record(s32 id, s32 a, s32 b, s32 c) {
    g_Events[g_EventCount++] = (Event){id, a, b, c};
}

void DrawAnimatedScenery(s32 timer, s32 variant) {
    Record(EVENT_ANIMATED, timer, variant, 0);
}

void DrawAnimatedScenery2(s32 timer, s32 variant, s32 night, s32 animate) {
    Record(EVENT_ANIMATED_2, timer, variant, night * 10 + animate);
}

void DrawSpinningScenery(s32 timer, s32 animate) {
    Record(EVENT_SPINNING, timer, animate, 0);
}

void DrawHighClassScenery(void) {
    Record(EVENT_HIGH_CLASS, 0, 0, 0);
}

void DrawStaticScenery(s32 shifted) {
    Record(EVENT_STATIC, shifted, 0, 0);
}

void UpdateShuttleScenery(s32 index) {
    Record(EVENT_UPDATE_SHUTTLE, index, 0, 0);
}

void DrawShuttleScenery(s32 index) {
    Record(EVENT_DRAW_SHUTTLE, index, 0, 0);
}

static int Expect(const char *label, const Event *expected, s32 count) {
    s32 index;

    if (g_EventCount != count) {
        printf("FAIL %s: %d events, expected %d\n",
               label, g_EventCount, count);
        return 0;
    }
    for (index = 0; index < count; index++) {
        Event *actual = &g_Events[index];
        if (actual->id != expected[index].id ||
            actual->a != expected[index].a ||
            actual->b != expected[index].b ||
            actual->c != expected[index].c) {
            printf("FAIL %s event %d: (%d,%d,%d,%d)\n", label, index,
                   actual->id, actual->a, actual->b, actual->c);
            return 0;
        }
    }
    g_EventCount = 0;
    return 1;
}

int main(void) {
    static const Event course0[] = {
        {EVENT_ANIMATED, 9, 0, 0},
        {EVENT_SPINNING, 9, 1, 0},
        {EVENT_HIGH_CLASS, 0, 0, 0},
        {EVENT_STATIC, 0, 0, 0},
    };
    static const Event course1[] = {
        {EVENT_ANIMATED, 8, 0, 0},
        {EVENT_UPDATE_SHUTTLE, 0, 0, 0},
        {EVENT_DRAW_SHUTTLE, 0, 0, 0},
        {EVENT_STATIC, 0, 0, 0},
    };
    static const Event course2[] = {
        {EVENT_ANIMATED, 7, 0, 0},
        {EVENT_UPDATE_SHUTTLE, 0, 0, 0},
        {EVENT_UPDATE_SHUTTLE, 1, 0, 0},
        {EVENT_DRAW_SHUTTLE, 0, 0, 0},
        {EVENT_DRAW_SHUTTLE, 1, 0, 0},
        {EVENT_STATIC, 0, 0, 0},
    };
    static const Event course3[] = {
        {EVENT_ANIMATED, 6, 0, 0},
        {EVENT_ANIMATED, 6, 1, 0},
        {EVENT_STATIC, 1, 0, 0},
    };
    static const Event class5[] = {
        {EVENT_ANIMATED, 5, 0, 0},
        {EVENT_SPINNING, 5, 0, 0},
        {EVENT_DRAW_SHUTTLE, 0, 0, 0},
        {EVENT_STATIC, 0, 0, 0},
    };
    static const Event unknownCourse[] = {
        {EVENT_ANIMATED, 3, 0, 0},
    };
    static const Event secondPass[] = {
        {EVENT_ANIMATED_2, 4, 0, 10},
        {EVENT_ANIMATED_2, 4, 1, 10},
        {EVENT_STATIC, 1, 0, 0},
    };

    g_GrandPrixClass = 4;
    DrawCourseScenery(0, 9, 1);
    if (!Expect("course 0", course0, 4)) return 1;

    g_GrandPrixClass = 1;
    DrawCourseScenery(1, 8, 1);
    if (!Expect("course 1", course1, 4)) return 1;

    DrawCourseScenery(2, 7, 1);
    if (!Expect("course 2", course2, 6)) return 1;

    DrawCourseScenery(3, 6, 1);
    if (!Expect("course 3", course3, 3)) return 1;

    g_GrandPrixClass = 5;
    DrawCourseScenery(1, 5, 1);
    if (!Expect("class 5", class5, 4)) return 1;

    DrawCourseScenery(99, 3, 1);
    if (!Expect("unknown course", unknownCourse, 1)) return 1;

    g_CourseIndex = 3;
    g_SceneId = 0x11;
    DrawCourseScenery2(4, 1);
    if (!Expect("second pass", secondPass, 3)) return 1;

    puts("course scenery dispatch preserved");
    return 0;
}
