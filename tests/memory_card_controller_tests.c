#include "game/memory_card_controller.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    SaveSession state = {3, 0, 0, 0, 0, 0, -1, 0, 3};
    int index;
    MemoryCardActionSession action;
    MemoryCardCursorResult cursor;
    EXPECT_EQ(1, MemoryCardControllerShouldPoll(0, 0));
    EXPECT_EQ(0, MemoryCardControllerShouldPoll(1, 0));
    EXPECT_EQ(1, MemoryCardControllerShouldPoll(1, 1));

    for (index = 0; index < 6; index++)
        MemoryCardControllerApplyStatus(&state, 0);
    EXPECT_EQ(0, state.selection);
    MemoryCardControllerApplyStatus(&state, 0);
    EXPECT_EQ(3, state.selection);

    MemoryCardControllerApplyStatus(&state, 1);
    EXPECT_EQ(1, state.selection);
    EXPECT_EQ(2, state.subState);
    MemoryCardControllerResolveDetection(&state);
    EXPECT_EQ(2, state.menuState);

    state = (SaveSession){3, -3, 0, -3, 0, 4, -1, 0, 3};
    MemoryCardControllerResolveDetection(&state);
    EXPECT_EQ(-3, state.menuState);
    EXPECT_EQ(0, state.errorTicks);

    state = (SaveSession){1, 3, 0, 3, 0, 0, -1, 0, 3};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(3, state.menuState);
    EXPECT_EQ(1, state.lastMenuState);

    state = (SaveSession){-1, 1, 0, 1, 0, 0, 0, 1, 1};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(2, state.menuState);
    EXPECT_EQ(0, state.errorPending);
    EXPECT_EQ(3, state.errorCountdown);

    state = (SaveSession){2, -3, 0, -3, 0, 0, 0, 0, 2};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(2, state.menuState);
    EXPECT_EQ(1, state.errorPending);
    EXPECT_EQ(1, state.errorCountdown);
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(-3, state.menuState);

    state = (SaveSession){-2, -1, 0, -1, 0, 0, 0, 0, 3};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(-1, state.menuState);

    action = (MemoryCardActionSession){MC_ACTION_SAVE_DELAY, 2, 0, 1, 1, 0, 0};
    MemoryCardActionTick(&action, 2);
    EXPECT_EQ(MC_ACTION_SAVE_DELAY, action.state);
    MemoryCardActionTick(&action, 2);
    EXPECT_EQ(MC_ACTION_SAVE_WRITE, action.state);
    EXPECT_EQ(MC_ACTION_EFFECT_WRITE_SLOT,
              MemoryCardActionRequestedEffect(&action));

    action = (MemoryCardActionSession){MC_ACTION_SAVE_WAIT_CARD, 0, 0, 1, 1, 0, 0};
    for (index = 0; index < 3; index++)
        MemoryCardActionObserveCard(&action, 1);
    EXPECT_EQ(MC_ACTION_SAVE_WAIT_CARD, action.state);
    MemoryCardActionObserveCard(&action, 1);
    EXPECT_EQ(MC_ACTION_SAVE_SHOW_RESULT, action.state);

    MemoryCardActionShowResult(&action, 1, 1);
    EXPECT_EQ(MC_ACTION_SAVE_RESULT_DELAY, action.state);
    EXPECT_EQ(0x12, action.prompt);
    EXPECT_EQ(0x3C, action.timer);
    action.timer = 1;
    MemoryCardActionTick(&action, 2);
    EXPECT_EQ(MC_ACTION_IDLE, action.state);
    EXPECT_EQ(0, action.menuPage);
    EXPECT_EQ(2, action.menuRowCursor);

    action = (MemoryCardActionSession){MC_ACTION_LOAD_WAIT_CARD, 0, 3, 1, 1, 0, 0};
    EXPECT_EQ(MC_ACTION_EFFECT_POLL_CARD,
              MemoryCardActionRequestedEffect(&action));
    MemoryCardActionObserveCard(&action, 0);
    EXPECT_EQ(MC_ACTION_LOAD_WAIT_CARD, action.state);
    MemoryCardActionObserveCard(&action, 1);
    EXPECT_EQ(MC_ACTION_LOAD_SHOW_RESULT, action.state);
    MemoryCardActionShowResult(&action, 0, 0);
    EXPECT_EQ(MC_ACTION_LOAD_RESULT_DELAY, action.state);
    EXPECT_EQ(0x10, action.prompt);

    cursor = MemoryCardMoveMenuRow(1, 0, 2, PAD_UP | PAD_DOWN);
    EXPECT_EQ(2, cursor.value);
    EXPECT_EQ(1, cursor.moved);
    cursor = MemoryCardMoveMenuRow(2, 0, 2, PAD_DOWN);
    EXPECT_EQ(2, cursor.value);
    EXPECT_EQ(0, cursor.moved);
    cursor = MemoryCardSetBinaryChoice(0, PAD_LEFT | PAD_RIGHT);
    EXPECT_EQ(1, cursor.value);
    EXPECT_EQ(1, cursor.moved);
    return 0;
}
