#include "game/menu_controller.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    MenuCursorResult result;

    result = MenuCursorMove(0, 6, -1, 0);
    EXPECT_EQ(5, result.selection);
    EXPECT_EQ(1, result.moved);
    result = MenuCursorMove(5, 6, 1, 0);
    EXPECT_EQ(0, result.selection);

    /* The locked Extra GP row is skipped in either direction. */
    result = MenuCursorMove(0, 5, 1, 0x1Du);
    EXPECT_EQ(2, result.selection);
    result = MenuCursorMove(2, 5, -1, 0x1Du);
    EXPECT_EQ(0, result.selection);

    result = MenuCursorMove(2, 4, 0, 0);
    EXPECT_EQ(2, result.selection);
    EXPECT_EQ(0, result.moved);
    result = MenuCursorMove(1, 3, 1, 1u << 1);
    EXPECT_EQ(1, result.selection);
    EXPECT_EQ(0, result.moved);

    EXPECT_EQ(MENU_ACTION_CONFIRM,
              MenuResolveAction(PAD_CONFIRM | PAD_CANCEL,
                                PAD_CONFIRM, PAD_CANCEL));
    EXPECT_EQ(MENU_ACTION_CANCEL,
              MenuResolveAction(PAD_CANCEL, PAD_CONFIRM, PAD_CANCEL));
    EXPECT_EQ(MENU_ACTION_NONE,
              MenuResolveAction(PAD_LEFT, PAD_CONFIRM, PAD_CANCEL));
    return 0;
}
