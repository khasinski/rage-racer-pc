#include "game/car_control_command.h"
#include "game/game_input.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    GameInputFrame input = {0};
    CarControlCommand command;

    input.controllerType = 0x41;
    input.accelerateHeld = 1;
    input.brakeHeld = 1;
    input.shiftUp = 1;
    input.steerLeft = 1;
    command = CarControlCommandBuildPlayer(&input, 0, 1);
    EXPECT_EQ(256, command.accelerator);
    EXPECT_EQ(256, command.brake);
    EXPECT_EQ(1, command.shiftUp);
    EXPECT_EQ(1, command.steerLeft);
    EXPECT_EQ(1, command.digitalController);
    EXPECT_EQ(0, command.analogController);

    input.controllerType = 0x23;
    input.analogI = 106;
    input.analogII = 53;
    input.analogL = 106;
    input.steering = -37;
    command = CarControlCommandBuildPlayer(&input, 0, 1);
    EXPECT_EQ(256, command.accelerator);
    EXPECT_EQ(128, command.brake);
    EXPECT_EQ(1, command.analogController);
    EXPECT_EQ(0, command.digitalController);
    EXPECT_EQ(-37, command.steering);
    command = CarControlCommandBuildPlayer(&input, 1, 1);
    EXPECT_EQ(128, command.accelerator);
    EXPECT_EQ(256, command.brake);
    command = CarControlCommandBuildPlayer(&input, 2, 1);
    EXPECT_EQ(256, command.accelerator);
    EXPECT_EQ(256, command.brake);
    command = CarControlCommandBuildPlayer(&input, 3, 1);
    EXPECT_EQ(128, command.accelerator);
    EXPECT_EQ(256, command.brake);

    command = CarControlCommandBuildPlayer(&input, 0, 0);
    EXPECT_EQ(0, command.accelerator);
    EXPECT_EQ(0, command.brake);
    EXPECT_EQ(1, command.shiftUp);
    command = CarControlCommandBuildRival(123, 1);
    EXPECT_EQ(CAR_CONTROL_RIVAL, command.source);
    EXPECT_EQ(123, command.targetYaw);
    EXPECT_EQ(1, command.boostEnabled);
    return 0;
}
