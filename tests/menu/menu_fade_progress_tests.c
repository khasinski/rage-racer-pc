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

    puts("menu animation helper tests passed");
    return 0;
}
