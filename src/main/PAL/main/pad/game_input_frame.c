#include "game/game_input.h"

void GameInputBuild(GameInputFrame *frame, const GameInputRawState *raw,
                    const u16 buttonMapping[16]) {
    s32 mappingOffset = raw->controllerType == 0x23 ? 8 : 0;

    frame->held = raw->held;
    frame->pressed = raw->pressed;
    frame->pressedRepeat = raw->pressedRepeat;
    frame->controllerType = raw->controllerType;
    frame->steering = raw->steering;
    frame->analogI = raw->analogI;
    frame->analogII = raw->analogII;
    frame->analogL = raw->analogL;
    frame->confirm = (raw->pressed & PAD_CONFIRM) != 0;
    frame->cancel = (raw->pressed & PAD_CANCEL) != 0;
    frame->pause = (raw->pressed & PAD_START) != 0;
    frame->shiftUp =
        (raw->pressed & buttonMapping[mappingOffset + 4]) != 0;
    frame->shiftDown =
        (raw->pressed & buttonMapping[mappingOffset + 5]) != 0;
    frame->steerLeft =
        (raw->held & buttonMapping[mappingOffset]) != 0;
    frame->steerRight =
        (raw->held & buttonMapping[mappingOffset + 1]) != 0;
    frame->accelerateHeld =
        (raw->held & buttonMapping[mappingOffset + 2]) != 0;
    frame->brakeHeld =
        (raw->held & buttonMapping[mappingOffset + 3]) != 0;
}
