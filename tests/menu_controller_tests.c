#include "game/menu_controller.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    MenuCursorResult result;
    MenuSession session;
    MenuSessionCommands commands;

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

    session = (MenuSession){0, 5, 0x1Du};
    commands = MenuSessionStep(&session, 1, PAD_DOWN);
    EXPECT_EQ(2, session.selection);
    EXPECT_EQ(1, commands.moved);
    EXPECT_EQ(MENU_ACTION_NONE, commands.action);
    commands = MenuSessionStep(&session, 0, PAD_CONFIRM | PAD_CANCEL);
    EXPECT_EQ(MENU_ACTION_CONFIRM, commands.action);
    session = (MenuSession){0, 3, 0};
    commands = MenuSessionStepVertical(&session, PAD_UP | PAD_DOWN);
    EXPECT_EQ(0, session.selection);
    EXPECT_EQ(2, commands.moveCount);

    EXPECT_EQ(1, MenuViewIsSettled(100, 110, 10));
    EXPECT_EQ(0, MenuViewIsSettled(100, 111, 10));
    EXPECT_EQ(1, MenuViewIsSettled(110, 100, 10));
    EXPECT_EQ(0, MenuViewIsSettled(100, 100, -1));
    EXPECT_EQ(1, MenuExitIsReady(0, 101, 100));
    EXPECT_EQ(0, MenuExitIsReady(1, 101, 100));
    EXPECT_EQ(0, MenuExitIsReady(0, 100, 100));

    return 0;
}
