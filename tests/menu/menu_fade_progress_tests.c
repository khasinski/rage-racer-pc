#include "game/menu.h"
#include "game/menu_internal.h"

#include <limits.h>
#include <stdio.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    s32 progress = 100;

    CHECK(AdvanceMenuFade(&progress, 20) == 120);
    CHECK(AdvanceMenuFade(&progress, -40) == 80);
    CHECK(AdvanceMenuFade(&progress, INT_MAX) == MENU_FADE_MAX);
    CHECK(AdvanceMenuFade(&progress, INT_MIN) == 0);

    progress = MENU_FADE_MAX;
    CHECK(AdvanceMenuFade(&progress, 1) == MENU_FADE_MAX);
    progress = 1;
    CHECK(AdvanceMenuFade(&progress, -2) == 0);

    progress = 123;
    CHECK(AdvanceMenuFade(&progress, 0) == 0);

    CHECK(MenuValueWithinWindow(100, 110, 10));
    CHECK(MenuValueWithinWindow(110, 100, 10));
    CHECK(!MenuValueWithinWindow(100, 111, 10));
    CHECK(!MenuValueWithinWindow(INT_MIN, INT_MAX, UINT_MAX - 1));
    CHECK(MenuValueWithinWindow(INT_MIN, INT_MAX, UINT_MAX));
    CHECK(MenuValueWithinWindow(INT_MIN, INT_MIN, 0));

    CHECK(MenuWrapAngle(0, 600000) == 0);
    CHECK(MenuWrapAngle(299999, 600000) == 299999);
    CHECK(MenuWrapAngle(300000, 600000) == -300000);
    CHECK(MenuWrapAngle(-300001, 600000) == 299999);
    CHECK(MenuWrapAngle(900000, 600000) == -300000);
    CHECK(MenuWrapAngle(INT_MIN, 600000) == -83648);
    CHECK(MenuWrapAngle(INT_MAX, 600000) == 83647);
    CHECK(MenuWrapAngle(123, 0) == 0);

    CHECK(AdvanceMenuViewOffsetValue(2500, 250000) == 2813);
    CHECK(AdvanceMenuViewOffsetValue(250000, 0) == 229166);
    CHECK(AdvanceMenuViewOffsetValue(42, 42) == 42);
    CHECK(AdvanceMenuViewOffsetValue(INT_MIN, INT_MAX) == INT_MIN);
    CHECK(AdvanceMenuViewOffsetValue(INT_MAX, INT_MIN) == 1789569705);

    CHECK(AdvanceMenuViewAngleValue(100, 100, 24) == 100);
    CHECK(AdvanceMenuViewAngleValue(100, 124, 24) == 102);
    CHECK(AdvanceMenuViewAngleValue(100, 101, 24) == 101);
    CHECK(AdvanceMenuViewAngleValue(100, 76, 24) == 98);
    CHECK(AdvanceMenuViewAngleValue(100, 200, 0) == 100);
    CHECK(AdvanceMenuViewAngleValue(INT_MIN, INT_MAX, 24) > INT_MIN);
    CHECK(AdvanceMenuViewAngleValue(INT_MAX, INT_MIN, 24) < INT_MAX);

    puts("menu animation helper tests passed");
    return 0;
}
