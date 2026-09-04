#include "game/car_render_rules.h"

#include <limits.h>
#include <stdio.h>

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            fprintf(stderr, "%s:%d: got %d, expected %d\n", __FILE__,        \
                    __LINE__, (s32)(actual), (s32)(expected));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    static const MirrorBadgeStyle badgeStyles[] = {
        MIRROR_BADGE_STYLE_NARROW,
        MIRROR_BADGE_STYLE_WIDE,
        MIRROR_BADGE_STYLE_MEDIUM,
        MIRROR_BADGE_STYLE_LARGE,
        MIRROR_BADGE_STYLE_COUNT,
    };

    CHECK_EQ(CarRenderManhattanDistance(100, -50, 70, -20), 60);
    CHECK_EQ(CarRenderManhattanDistance(-100, 50, -70, 20), 60);
    CHECK_EQ(CarRenderManhattanDistance(INT_MIN, 0, INT_MAX, 0), INT_MAX);
    CHECK_EQ(CarRenderManhattanDistance(INT_MIN, INT_MIN,
                                        INT_MIN, INT_MIN), 0);

    CHECK_EQ(ClassifyCarRenderRange(-1, 0), CAR_RENDER_BEHIND);
    CHECK_EQ(ClassifyCarRenderRange(0, 0xCFF), CAR_RENDER_CLOSE);
    CHECK_EQ(ClassifyCarRenderRange(0, 0xD00), CAR_RENDER_FAR);
    CHECK_EQ(ClassifyCarRenderRange(0, 0x24FF), CAR_RENDER_FAR);
    CHECK_EQ(ClassifyCarRenderRange(0, 0x2500), CAR_RENDER_CULLED);

    CHECK_EQ(ResolveCarModelBank(3, 2, 6), 5);
    CHECK_EQ(ResolveCarModelBank(3, 3, 6), 1);
    CHECK_EQ(ResolveCarModelBank(6, 0, 6), 1);
    CHECK_EQ(ResolveCarModelBank(-1, 0, 6), 1);
    CHECK_EQ(ResolveCarModelBank(INT_MAX, 1, 6), 1);
    CHECK_EQ(ResolveCarModelBank(INT_MIN, -1, 6), 1);

    CHECK_EQ(ResolveMirrorBadgeStyle(0, badgeStyles, 5),
             MIRROR_BADGE_STYLE_NARROW);
    CHECK_EQ(ResolveMirrorBadgeStyle(1, badgeStyles, 5),
             MIRROR_BADGE_STYLE_WIDE);
    CHECK_EQ(ResolveMirrorBadgeStyle(2, badgeStyles, 5),
             MIRROR_BADGE_STYLE_MEDIUM);
    CHECK_EQ(ResolveMirrorBadgeStyle(3, badgeStyles, 5),
             MIRROR_BADGE_STYLE_LARGE);
    CHECK_EQ(ResolveMirrorBadgeStyle(4, badgeStyles, 5),
             MIRROR_BADGE_STYLE_NARROW);
    CHECK_EQ(ResolveMirrorBadgeStyle(-1, badgeStyles, 5),
             MIRROR_BADGE_STYLE_NARROW);
    CHECK_EQ(ResolveMirrorBadgeStyle(5, badgeStyles, 5),
             MIRROR_BADGE_STYLE_NARROW);
    CHECK_EQ(ResolveMirrorBadgeStyle(0, NULL, 5),
             MIRROR_BADGE_STYLE_NARROW);
    CHECK_EQ(ResolveMirrorBadgeStyle(1, badgeStyles, -1),
             MIRROR_BADGE_STYLE_NARROW);

    CHECK_EQ(AdvanceMirrorPanelY(-44, 0), -44);
    CHECK_EQ(AdvanceMirrorPanelY(-43, 0), -44);
    CHECK_EQ(AdvanceMirrorPanelY(18, 0), 17);
    CHECK_EQ(AdvanceMirrorPanelY(17, 1), 18);
    CHECK_EQ(AdvanceMirrorPanelY(18, 1), 18);
    CHECK_EQ(AdvanceMirrorPanelY(-44, 1), -43);

    puts("car render rules tests passed");
    return 0;
}
