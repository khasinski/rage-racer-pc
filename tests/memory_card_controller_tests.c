#include "game/memory_card_controller.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    MemoryCardControllerState state = {3, 0, 0, 0, 0, 0, -1, 0, 3};
    int index;
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

    state = (MemoryCardControllerState){3, -3, 0, -3, 0, 4, -1, 0, 3};
    MemoryCardControllerResolveDetection(&state);
    EXPECT_EQ(-3, state.menuState);
    EXPECT_EQ(0, state.errorTicks);

    state = (MemoryCardControllerState){1, 3, 0, 3, 0, 0, -1, 0, 3};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(3, state.menuState);
    EXPECT_EQ(1, state.lastMenuState);

    state = (MemoryCardControllerState){-1, 1, 0, 1, 0, 0, 0, 1, 1};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(2, state.menuState);
    EXPECT_EQ(0, state.errorPending);
    EXPECT_EQ(3, state.errorCountdown);

    state = (MemoryCardControllerState){2, -3, 0, -3, 0, 0, 0, 0, 2};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(2, state.menuState);
    EXPECT_EQ(1, state.errorPending);
    EXPECT_EQ(1, state.errorCountdown);
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(-3, state.menuState);

    state = (MemoryCardControllerState){-2, -1, 0, -1, 0, 0, 0, 0, 3};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(-1, state.menuState);
    return 0;
}
