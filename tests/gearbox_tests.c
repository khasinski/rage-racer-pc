#include "game/gearbox.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    GameCarSpecShiftPoint points[6] = {
        {0, 100}, {50, 200}, {150, 300}, {250, 400}, {350, 500}, {450, 600}};
    GearboxState state = {1, 0, 1, 0, CAR_MOTION_DRIVING, 0, 8};
    GearboxInput input = {0, 0, 1, 0, 6, points};

    GearboxUpdate(&state, &input);
    EXPECT_EQ(2, state.gear);
    EXPECT_EQ(0, state.steerHoldFrames);
    input.shiftUpPressed = input.shiftDownPressed = 1;
    GearboxUpdate(&state, &input);
    EXPECT_EQ(2, state.gear);

    state.manual = 0;
    state.gear = 2;
    state.steerHoldFrames = 9;
    input.shiftUpPressed = input.shiftDownPressed = 0;
    input.speed = 201;
    GearboxUpdate(&state, &input);
    EXPECT_EQ(3, state.gear);
    EXPECT_EQ(24, state.autoShiftCooldown);
    EXPECT_EQ(0, state.steerHoldFrames);

    state.brakeInput = 129;
    GearboxUpdate(&state, &input);
    EXPECT_EQ(22, state.autoShiftCooldown);
    state.autoShiftCooldown = 0;
    input.speed = 149;
    GearboxUpdate(&state, &input);
    EXPECT_EQ(2, state.gear);
    EXPECT_EQ(23, state.autoShiftCooldown);

    input.speed = 0;
    state.gear = 4;
    state.clutch = 7;
    state.motionState = CAR_MOTION_DRIVING;
    GearboxUpdate(&state, &input);
    EXPECT_EQ(1, state.gear);
    EXPECT_EQ(0, state.clutch);
    EXPECT_EQ(0, state.autoShiftCooldown);

    state.gear = 4;
    state.motionState = CAR_MOTION_STANDING_START;
    GearboxUpdate(&state, &input);
    /* Standing-start protection skips the forced reset, but ordinary
     * automatic downshift logic still runs first. */
    EXPECT_EQ(3, state.gear);
    return 0;
}
