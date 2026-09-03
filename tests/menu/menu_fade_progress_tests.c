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

    CHECK(AddClampedMenuValue(10, 5, 0, 20) == 15);
    CHECK(AddClampedMenuValue(INT_MAX, INT_MAX, 0, 20) == 20);
    CHECK(AddClampedMenuValue(INT_MIN, INT_MIN, 0, 20) == 0);
    CHECK(AddClampedMenuValue(10, 5, 20, 0) == 20);
    CHECK(WrapMenuIndex(0, -1, 3) == 2);
    CHECK(WrapMenuIndex(2, 1, 3) == 0);
    CHECK(WrapMenuIndex(INT_MAX, INT_MAX, 20) == 14);
    CHECK(WrapMenuIndex(INT_MIN, INT_MIN, 20) == 4);
    CHECK(WrapMenuIndex(5, 1, 0) == 0);

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
    CHECK(AdvanceMenuViewOffsetValue(INT_MIN, INT_MAX) == 1);
    CHECK(AdvanceMenuViewOffsetValue(INT_MAX, INT_MIN) == 229166);
    CHECK(NormalizeMenuViewOffset(-1) == MENU_VIEW_OFFSET_MIN);
    CHECK(NormalizeMenuViewOffset(INT_MAX) == MENU_VIEW_OFFSET_MAX);
    CHECK(NormalizeCourseSwapDelay(INT_MIN) == 0);
    CHECK(NormalizeCourseSwapDelay(7) == 7);
    CHECK(NormalizeCourseSwapDelay(INT_MAX) == 19);

    CHECK(AdvanceMenuViewAngleValue(100, 100, 24) == 100);
    CHECK(AdvanceMenuViewAngleValue(100, 124, 24) == 102);
    CHECK(AdvanceMenuViewAngleValue(100, 101, 24) == 101);
    CHECK(AdvanceMenuViewAngleValue(100, 76, 24) == 98);
    CHECK(AdvanceMenuViewAngleValue(100, 200, 0) == 100);
    CHECK(AdvanceMenuViewAngleValue(INT_MIN, INT_MAX, 24) > INT_MIN);
    CHECK(AdvanceMenuViewAngleValue(INT_MAX, INT_MIN, 24) < INT_MAX);

    CHECK(RebaseCarouselValue(700000, 500000, 600000) == 800000);
    CHECK(RebaseCarouselValue(INT_MAX, -1, 0) == INT_MIN);
    CHECK(RebaseCarouselValue(INT_MIN, 1, -1) == INT_MAX - 1);

    CHECK(TeamNameCharacterModelIndex(0, 44) == 0);
    CHECK(TeamNameCharacterModelIndex(10, 44) == -1);
    CHECK(TeamNameCharacterModelIndex(42, 44) == -1);
    CHECK(TeamNameCharacterModelIndex(43, 44) == -1);
    CHECK(TeamNameCharacterModelIndex(-1, 44) == 1);
    CHECK(TeamNameCharacterModelIndex(44, 44) == 1);
    CHECK(TeamNameCharacterModelIndex(INT_MAX, 1) == 0);
    CHECK(TeamNameCharacterModelIndex(0, 0) == -1);
    CHECK(NormalizeTeamNameCursor(-1) == 0);
    CHECK(NormalizeTeamNameCursor(INT_MAX) == TEAM_NAME_KEY_END);
    CHECK(NormalizeTeamNameCursor(TEAM_NAME_KEY_RUBOUT) ==
          TEAM_NAME_KEY_RUBOUT);
    CHECK(NormalizeTeamNameCursor(5) == 5);
    CHECK(MenuModelIndexOrFallback(5, 6) == 5);
    CHECK(MenuModelIndexOrFallback(5, 2) == 1);
    CHECK(MenuModelIndexOrFallback(-1, 2) == 1);
    CHECK(MenuModelIndexOrFallback(5, 1) == 0);
    CHECK(MenuModelIndexOrFallback(0, 0) == -1);

    {
        const s32 prices[] = {0, 1200, -1};
        ShopPrice price;

        price = LookupShopPrice(prices, 3, 1);
        CHECK(price.available && price.amount == 1200);
        price = LookupShopPrice(prices, 3, 0);
        CHECK(price.available && price.amount == 0);
        CHECK(!LookupShopPrice(prices, 3, 2).available);
        CHECK(!LookupShopPrice(prices, 3, -1).available);
        CHECK(!LookupShopPrice(NULL, 3, 0).available);
    }

    CHECK(!ShowroomCarAtSwapPoint(300000, 0, 2));
    CHECK(ShowroomCarAtSwapPoint(299999, 0, 2));
    CHECK(!ShowroomCarAtSwapPoint(900000, 1000000, 2));
    CHECK(ShowroomCarAtSwapPoint(900001, 1000000, 2));
    CHECK(!ShowroomCarAtSwapPoint(299999, 0, -1));
    CHECK(!ShowroomCarAtSwapPoint(100, 100, 2));

    CHECK(!CourseCarouselAtSwapPoint(750000, 1000000, 2));
    CHECK(CourseCarouselAtSwapPoint(750001, 1000000, 2));
    CHECK(!CourseCarouselAtSwapPoint(250000, 0, 2));
    CHECK(CourseCarouselAtSwapPoint(249999, 0, 2));
    CHECK(!CourseCarouselAtSwapPoint(249999, 0, -1));
    CHECK(!CourseCarouselAtSwapPoint(100, 100, 2));

    CHECK(UpdatedMenuViewSpin(0, PAD_L1) == 1);
    CHECK(UpdatedMenuViewSpin(0, PAD_R1) == -1);
    CHECK(UpdatedMenuViewSpin(64, PAD_L1) == 64);
    CHECK(UpdatedMenuViewSpin(-64, PAD_R1) == -64);
    CHECK(UpdatedMenuViewSpin(64, PAD_L1 | PAD_R1) == 64);
    CHECK(UpdatedMenuViewSpin(INT_MAX, 0) == 64);

    CHECK(UpdatedShowroomSteering(0, PAD_R2) == 192);
    CHECK(UpdatedShowroomSteering(0, PAD_L2) == -192);
    CHECK(UpdatedShowroomSteering(6143, PAD_R2) == 6144);
    CHECK(UpdatedShowroomSteering(-6143, PAD_L2) == -6144);
    CHECK(UpdatedShowroomSteering(500, PAD_L2 | PAD_R2) == 500);
    CHECK(UpdatedShowroomSteering(INT_MIN, 0) == -6144);

    puts("menu animation helper tests passed");
    return 0;
}
