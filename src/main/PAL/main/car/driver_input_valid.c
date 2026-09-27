#include "game/car_control.h"

static int IsFlag(int value) {
    return value == 0 || value == 1;
}

int ValidDriverInput(const DriverInput *input) {
    return input != 0 && input->throttle >= 0 && input->throttle <= 256 &&
        input->brake >= 0 && input->brake <= 256 &&
        input->steering.mode >= STEERING_CENTER && input->steering.mode <= STEERING_ANALOG &&
        input->steering.angle >= -(13 * 512) && input->steering.angle <= 13 * 512 &&
        IsFlag(input->steering.left) && IsFlag(input->steering.right) &&
        IsFlag(input->shiftUp) && IsFlag(input->shiftDown);
}

