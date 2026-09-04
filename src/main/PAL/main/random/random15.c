#include "game/diagnostics.h"
#include "game/random.h"
#include "game/state.h"

#include <inttypes.h>
#include <stdio.h>

enum {
    RANDOM_MULTIPLIER = 0x41C64E6D,
    RANDOM_INCREMENT = 0x3039,
    RANDOM_RESULT_MASK = 0x7FFF,
};

static void TraceRandom15(u32 seed, s32 result, void *caller) {
    static unsigned long long traceIndex;
    static int traceEnabled = -1;

    if (traceEnabled < 0) {
        traceEnabled = DiagnosticsEnabled("random.trace");
    }
    if (traceEnabled) {
        intptr_t callerDelta = (intptr_t)(
            (uintptr_t)caller - (uintptr_t)(void *)&Random15);

        printf("random index=%llu frame=%d scene=%d timer=%d "
               "caller_delta=%" PRIdPTR " seed=%08x value=%04x\n",
               traceIndex, g_FrameCounter, g_SceneId, g_SceneTimer,
               callerDelta,
               seed, result);
    }
    traceIndex++;
}

s32 Random15(void) {
    u32 seed = g_RandomSeed * RANDOM_MULTIPLIER + RANDOM_INCREMENT;
    s32 result = (seed >> 16) & RANDOM_RESULT_MASK;

    g_RandomSeed = seed;
    TraceRandom15(seed, result, __builtin_return_address(0));
    return result;
}
