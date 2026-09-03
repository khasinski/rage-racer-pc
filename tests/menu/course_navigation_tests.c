#include "common.h"
#include "game/menu.h"

#include <limits.h>
#include <stdio.h>

s32 g_CourseIndex;
s16 g_ExtraGrandPrixUnlocked;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s32 g_MaxClassReached[2];
s16 g_SeriesSelection;

static int s_failures;

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

static void CheckGrandPrixLimits(void) {
    s32 extra;
    s32 classIndex;
    s32 course;

    g_GrandPrixMode = 1;
    for (extra = 0; extra <= 1; extra++) {
        for (classIndex = 1; classIndex <= 2; classIndex++) {
            s32 first = extra ? 4 : 0;
            s32 last = first + (classIndex < 2 ? 2 : 3);

            g_SeriesSelection = extra;
            g_GrandPrixClass = classIndex;
            for (course = 0; course <= 7; course++) {
                g_CourseIndex = course;
                Check("GP previous", CanSelectPrevCourse(),
                      course > first && course <= last);
                Check("GP next", CanSelectNextCourse(),
                      course >= first && course < last);
            }
        }
    }
}

static void CheckTimeAttackLimits(void) {
    s32 extraUnlocked;
    s32 classUnlocked;
    s32 course;

    g_GrandPrixMode = 0;
    g_SeriesSelection = 1; /* Selection does not constrain time attack. */
    for (extraUnlocked = 0; extraUnlocked <= 1; extraUnlocked++) {
        for (classUnlocked = 1; classUnlocked <= 2; classUnlocked++) {
            s32 first = 0;
            s32 last = (extraUnlocked ? 4 : 0) +
                       (classUnlocked < 2 ? 2 : 3);

            g_ExtraGrandPrixUnlocked = extraUnlocked;
            g_MaxClassReached[extraUnlocked] = classUnlocked;
            for (course = 0; course <= 7; course++) {
                g_CourseIndex = course;
                Check("TA previous", CanSelectPrevCourse(),
                      course > first && course <= last);
                Check("TA next", CanSelectNextCourse(),
                      course >= first && course < last);
            }
        }
    }
}

int main(void) {
    CheckGrandPrixLimits();
    CheckTimeAttackLimits();

    g_GrandPrixMode = 1;
    g_SeriesSelection = 1;
    g_GrandPrixClass = 2;
    g_CourseIndex = 0;
    Check("course below selected GP series has no previous",
          CanSelectPrevCourse(), 0);
    Check("course below selected GP series has no next",
          CanSelectNextCourse(), 0);
    g_CourseIndex = INT_MIN;
    Check("minimum course has no previous", CanSelectPrevCourse(), 0);
    Check("minimum course has no next", CanSelectNextCourse(), 0);
    g_CourseIndex = INT_MAX;
    Check("maximum course has no previous", CanSelectPrevCourse(), 0);
    Check("maximum course has no next", CanSelectNextCourse(), 0);

    return s_failures != 0;
}
