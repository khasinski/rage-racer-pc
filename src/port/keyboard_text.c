#include "keyboard_text.h"

#include <SDL3/SDL.h>
#include <string.h>

char PortConsumeTypedChar(void) {
    static unsigned char previous[512];
    int count = 0, limit, scancode;
    const bool *state = SDL_GetKeyboardState(&count);
    char result = 0;

    if (!state || count <= 0) return 0;
    limit = count < (int)sizeof(previous) ? count : (int)sizeof(previous);

    for (scancode = SDL_SCANCODE_A; scancode <= SDL_SCANCODE_Z && scancode < limit; ++scancode) {
        if (state[scancode] && !previous[scancode]) {
            result = (char)('A' + (scancode - SDL_SCANCODE_A));
            break;
        }
    }
    for (scancode = SDL_SCANCODE_1; !result && scancode <= SDL_SCANCODE_9 && scancode < limit; ++scancode) {
        if (state[scancode] && !previous[scancode]) {
            result = (char)('1' + (scancode - SDL_SCANCODE_1));
            break;
        }
    }
    if (!result && SDL_SCANCODE_0 < limit && state[SDL_SCANCODE_0] && !previous[SDL_SCANCODE_0])
        result = '0';
    if (!result && SDL_SCANCODE_SPACE < limit && state[SDL_SCANCODE_SPACE] && !previous[SDL_SCANCODE_SPACE])
        result = ' ';
    if (!result && SDL_SCANCODE_BACKSPACE < limit && state[SDL_SCANCODE_BACKSPACE] &&
        !previous[SDL_SCANCODE_BACKSPACE])
        result = '\b';
    if (!result && SDL_SCANCODE_PERIOD < limit && state[SDL_SCANCODE_PERIOD] && !previous[SDL_SCANCODE_PERIOD])
        result = '.';
    if (!result && SDL_SCANCODE_SEMICOLON < limit && state[SDL_SCANCODE_SEMICOLON] &&
        !previous[SDL_SCANCODE_SEMICOLON] &&
        ((SDL_SCANCODE_LSHIFT < limit && state[SDL_SCANCODE_LSHIFT]) ||
         (SDL_SCANCODE_RSHIFT < limit && state[SDL_SCANCODE_RSHIFT])))
        result = ':';

    for (scancode = 0; scancode < limit; ++scancode) previous[scancode] = state[scancode] ? 1 : 0;
    return result;
}
