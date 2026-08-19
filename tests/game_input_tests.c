#include "game/game_input.h"
#include "game/state.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                      \
    if ((int)(expected) != (int)(actual)) failures++;                         \
} while (0)

static void test_digital_mapping(void) {
    u16 mapping[16] = {0};
    GameInputFrame frame;
    GameInputRawState raw = {
        PAD_CROSS | PAD_SQUARE, PAD_START | PAD_TRIANGLE, PAD_RIGHT,
        0x41, -12, 40, 50, 60};
    mapping[0] = PAD_SQUARE;
    mapping[1] = PAD_CIRCLE;
    mapping[2] = PAD_CROSS;
    mapping[3] = PAD_CIRCLE;
    mapping[4] = PAD_TRIANGLE;
    mapping[5] = PAD_SQUARE;

    GameInputBuild(&frame, &raw, mapping);
    EXPECT_EQ(1, frame.confirm);
    EXPECT_EQ(1, frame.cancel);
    EXPECT_EQ(1, frame.pause);
    EXPECT_EQ(1, frame.shiftUp);
    EXPECT_EQ(0, frame.shiftDown);
    EXPECT_EQ(1, frame.steerLeft);
    EXPECT_EQ(0, frame.steerRight);
    EXPECT_EQ(1, frame.accelerateHeld);
    EXPECT_EQ(0, frame.brakeHeld);
    EXPECT_EQ(-12, frame.steering);
    EXPECT_EQ(40, frame.analogI);
}

static void test_negcon_uses_second_mapping_bank(void) {
    u16 mapping[16] = {0};
    GameInputFrame frame;
    GameInputRawState raw = {PAD_L1, PAD_R1, 0, 0x23, 0, 0, 0, 0};
    mapping[4] = PAD_R1;
    mapping[12] = PAD_L1;
    mapping[13] = PAD_R1;

    GameInputBuild(&frame, &raw, mapping);
    EXPECT_EQ(0, frame.shiftUp);
    EXPECT_EQ(1, frame.shiftDown);
}

int main(void) {
    test_digital_mapping();
    test_negcon_uses_second_mapping_bank();
    return failures != 0;
}
