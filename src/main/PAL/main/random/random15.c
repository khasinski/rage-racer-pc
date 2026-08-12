#include "common.h"
#include "game/random.h"

#ifdef __psyz
#include <stdlib.h>
#include "game/state.h"
#endif

s32 Random15(void) {
    u32 value = g_RandomSeed;
#ifdef __psyz
    static unsigned long long traceIndex;
    static int traceEnabled = -1;
    void *caller = __builtin_return_address(0);
#endif

    value *= 0x41C64E6D;
    value += 0x3039;
    g_RandomSeed = value;
#ifdef __psyz
    if (traceEnabled < 0) traceEnabled = getenv("RAGE_PORT_RANDOM_TRACE") != NULL;
    if (traceEnabled) {
        printf("random index=%llu frame=%d scene=%d timer=%d "
               "caller_delta=%td seed=%08x value=%04x\n",
               traceIndex, g_FrameCounter, g_SceneId, g_SceneTimer,
               (char *)caller - (char *)&Random15,
               value, (value >> 16) & 0x7FFF);
    }
    traceIndex++;
#endif
    return (value >> 0x10) & 0x7FFF;
}
