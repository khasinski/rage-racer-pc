#include "game/car_control_command.h"
#include "game/game_input.h"

static s32 ScaleNegconPedal(s32 value) {
    return (value << 8) / 106;
}

CarControlCommand CarControlCommandBuildPlayer(
    const struct GameInputFrame *input, s32 negconMappingIndex,
    s32 drivingEnabled) {
    CarControlCommand command = {0};

    command.source = CAR_CONTROL_PLAYER;
    command.shiftUp = input->shiftUp;
    command.shiftDown = input->shiftDown;
    command.steerLeft = input->steerLeft;
    command.steerRight = input->steerRight;
    command.steering = input->steering;
    command.digitalController = input->controllerType == 0x41;
    command.analogController = input->controllerType == 0x23;
    if (!drivingEnabled) return command;

    if (input->controllerType == 0x41) {
        command.accelerator = input->accelerateHeld << 8;
        command.brake = input->brakeHeld << 8;
        return command;
    }
    if (input->controllerType != 0x23) return command;

    command.accelerator = input->accelerateHeld << 8;
    command.brake = input->brakeHeld << 8;
    switch (negconMappingIndex) {
    case 0:
    case 5:
        command.accelerator = ScaleNegconPedal(input->analogI);
        command.brake = ScaleNegconPedal(input->analogII);
        break;
    case 1:
    case 6:
        command.accelerator = ScaleNegconPedal(input->analogII);
        command.brake = ScaleNegconPedal(input->analogI);
        break;
    case 2:
        command.brake = ScaleNegconPedal(input->analogL);
        break;
    case 3:
        command.accelerator = ScaleNegconPedal(input->analogII);
        command.brake = ScaleNegconPedal(input->analogL);
        break;
    default:
        break;
    }
    return command;
}

CarControlCommand CarControlCommandBuildRival(
    s32 targetYaw, s32 boostEnabled) {
    CarControlCommand command = {0};
    command.source = CAR_CONTROL_RIVAL;
    command.targetYaw = targetYaw;
    command.boostEnabled = boostEnabled != 0;
    return command;
}
