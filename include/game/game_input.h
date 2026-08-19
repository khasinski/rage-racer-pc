#ifndef GAME_GAME_INPUT_H
#define GAME_GAME_INPUT_H

#include "common.h"
#include "game/pad.h"

/* Immutable input snapshot consumed by one game-frame dispatch.  The raw
 * masks remain available while gameplay is migrated to semantic actions. */
typedef struct GameInputFrame {
    u16 held;
    u16 pressed;
    u16 pressedRepeat;
    u8 controllerType;
    s16 steering;
    s16 analogI;
    s16 analogII;
    s16 analogL;
    u8 confirm;
    u8 cancel;
    u8 pause;
    u8 shiftUp;
    u8 shiftDown;
    u8 steerLeft;
    u8 steerRight;
    u8 accelerateHeld;
    u8 brakeHeld;
} GameInputFrame;

typedef struct GameInputRawState {
    u16 held;
    u16 pressed;
    u16 pressedRepeat;
    u8 controllerType;
    s16 steering;
    s16 analogI;
    s16 analogII;
    s16 analogL;
} GameInputRawState;

extern GameInputFrame g_GameInput;

void GameInputBuild(GameInputFrame *frame, const GameInputRawState *raw,
                    const u16 buttonMapping[16]);
void GameInputCaptureLegacy(void);

#endif
