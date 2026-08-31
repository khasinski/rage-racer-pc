/* See host_clock.h for why the one call SDL is asked for lives on its own. */

#include "host_clock.h"

#include <SDL3/SDL_timer.h>

unsigned long long HostNanoseconds(void) {
    return (unsigned long long)SDL_GetTicksNS();
}
