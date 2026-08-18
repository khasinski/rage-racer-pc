#include "game/game_input.h"
#include "game/state.h"
#include "game/car.h"
#include "game/input_internal.h"

GameInputFrame g_GameInput;

void GameInputCaptureLegacy(void) {
    const GameInputRawState raw = {
        g_PadHeld, g_PadPressed, g_PadPressedRepeat, g_PadType,
        g_NegconSteer, g_NegconAnalogI, g_NegconAnalogII, g_NegconAnalogL};
    GameInputBuild(&g_GameInput, &raw, g_PadButtonMapping);
}
