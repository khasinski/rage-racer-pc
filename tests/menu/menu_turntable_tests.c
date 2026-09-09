#include "game/car.h"
#include "game/menu_internal.h"

#include <limits.h>
#include <stdio.h>

s32 g_CarListCursor;
s32 g_CarSwapFromIndex;
s32 g_CarSwapToIndex;
s32 g_MenuLowerAltPanelStep;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
s32 g_PlayerCarIndex;

static s32 s_lastRequestedCar;
static s32 s_requestCount;
static s32 s_requestResult;
static s32 s_soundCount;

s32 RequestCarModel(s32 carIndex) {
    s_lastRequestedCar = carIndex;
    s_requestCount++;
    return s_requestResult;
}

void PlaySoundCue(s32 cue) {
    if (cue == 8) {
        s_soundCount++;
    }
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(void) {
    s_lastRequestedCar = -1;
    s_requestCount = 0;
    s_requestResult = 1;
    s_soundCount = 0;
}

/* Advance the production showroom angle/swap predicates with an immediately
 * available model. Fast asset loading must not turn a short opposite press
 * into two car requests. */
static void AdvanceCarousel(void) {
    if (ShowroomCarAtSwapPoint(g_MenuViewAngle, g_MenuViewAngleTarget,
                              g_CarSwapToIndex)) {
        g_CarSwapFromIndex = g_CarSwapToIndex;
        g_CarSwapToIndex = -1;
    } else {
        g_MenuViewAngle = AdvanceMenuViewAngleValue(
            g_MenuViewAngle, g_MenuViewAngleTarget, 24);
    }
}

static int TestQuickDirectionChange(void) {
    for (int direction = -1; direction <= 1; direction += 2) {
        s32 shown = 5;
        s32 firstTarget = direction < 0 ? 0 : MENU_CAR_VIEW_RIGHT_TARGET;
        s32 secondTarget = direction < 0 ? MENU_CAR_VIEW_RIGHT_TARGET : 0;
        ResetCalls();
        g_MenuViewAngle = g_MenuViewAngleTarget = firstTarget;
        MenuSpinToCar(&shown, shown, shown + direction, firstTarget);
        for (int frame = 0; frame < 120 && g_CarSwapToIndex >= 0; ++frame)
            AdvanceCarousel();
        CHECK(g_CarSwapToIndex == -1 && MenuCarViewSettled());
        CHECK(shown == 5 + direction);

        ResetCalls();
        for (int frame = 0; frame < 6; ++frame) {
            AdvanceCarousel();
            if (MenuCarViewSettled() && g_CarSwapToIndex < 0)
                MenuSpinToCar(&shown, shown, shown - direction, secondTarget);
        }
        CHECK(s_requestCount == 1 && shown == 5);
        for (int frame = 0; frame < 120 && g_CarSwapToIndex >= 0; ++frame)
            AdvanceCarousel();
        CHECK(g_CarSwapToIndex == -1 && g_CarSwapFromIndex == 5);
    }
    return 0;
}

int main(void) {
    s32 shownCar = 2;

    if (TestQuickDirectionChange()) return 1;

    g_MenuViewAngleTarget = 500000;
    g_MenuViewAngle = 500000 + MENU_CAR_VIEW_SETTLE_WINDOW;
    CHECK(MenuCarViewSettled());
    g_MenuViewAngle++;
    CHECK(!MenuCarViewSettled());

    ResetCalls();
    g_MenuViewAngle = 700000;
    g_MenuViewAngleTarget = 500000;
    g_MenuLowerAltPanelStep = 5;
    MenuSpinToCar(&shownCar, 2, 6, 1200000);
    CHECK(shownCar == 6);
    CHECK(s_requestCount == 1 && s_lastRequestedCar == 6);
    CHECK(s_soundCount == 1);
    CHECK(g_CarSwapFromIndex == 2 && g_CarSwapToIndex == 6);
    CHECK(g_MenuViewAngleTarget == 1200000);
    CHECK(g_MenuViewAngle == 200000);
    CHECK(g_MenuLowerAltPanelStep == -1);

    ResetCalls();
    s_requestResult = 0;
    g_MenuViewAngle = 123;
    g_MenuViewAngleTarget = 456;
    g_MenuLowerAltPanelStep = 7;
    g_CarSwapFromIndex = 8;
    g_CarSwapToIndex = 9;
    MenuSpinToCar(&shownCar, 6, 3, 789);
    CHECK(s_requestCount == 1 && s_lastRequestedCar == 3);
    CHECK(s_soundCount == 0 && shownCar == 6);
    CHECK(g_MenuViewAngle == 123 && g_MenuViewAngleTarget == 456);
    CHECK(g_MenuLowerAltPanelStep == 7);
    CHECK(g_CarSwapFromIndex == 8 && g_CarSwapToIndex == 9);

    ResetCalls();
    MenuSpinToCar(NULL, 2, 3, 0);
    MenuSpinToCar(&shownCar, 2, -1, 0);
    MenuSpinToCar(&shownCar, 2, GAME_CAR_COUNT, 0);
    MenuSpinToCar(&shownCar, 2, INT_MAX, 0);
    CHECK(s_requestCount == 0 && s_soundCount == 0);
    CHECK(shownCar == 6);

    ResetCalls();
    g_PlayerCarIndex = 4;
    g_CarListCursor = 9;
    g_MenuViewAngle = 300000;
    g_MenuViewAngleTarget = 1200000;
    MenuSpinBackToPlayerCar();
    CHECK(s_requestCount == 1 && s_lastRequestedCar == 4);
    CHECK(g_CarSwapFromIndex == 9 && g_CarSwapToIndex == 4);
    CHECK(g_MenuViewAngleTarget == 0);
    CHECK(g_MenuViewAngle == -300000);

    ResetCalls();
    s_requestResult = 0;
    g_CarListCursor = 7;
    g_PlayerCarIndex = 4;
    g_MenuViewAngle = 123;
    g_MenuViewAngleTarget = 456;
    g_CarSwapFromIndex = 8;
    g_CarSwapToIndex = 9;
    MenuSpinBackToPlayerCar();
    CHECK(s_requestCount == 1 && s_lastRequestedCar == 4);
    CHECK(g_MenuViewAngle == 123 && g_MenuViewAngleTarget == 456);
    CHECK(g_CarSwapFromIndex == 8 && g_CarSwapToIndex == 9);

    ResetCalls();
    g_PlayerCarIndex = -1;
    MenuSpinBackToPlayerCar();
    g_PlayerCarIndex = GAME_CAR_COUNT;
    MenuSpinBackToPlayerCar();
    CHECK(s_requestCount == 0);

    puts("menu turntable tests passed");
    return 0;
}
