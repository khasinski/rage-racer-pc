#include "game/race_hud_internal.h"

#include <stdio.h>

static int Check(s32 first, s32 second, s32 timer,
                 s32 expectedFirst, s32 expectedSecond,
                 s32 brightness, s32 textOffset) {
    RaceOptionMarqueeState state =
        AdvanceRaceOptionMarquee(first, second, timer);

    if (state.firstScroll == expectedFirst &&
        state.secondScroll == expectedSecond &&
        state.brightness == brightness && state.textOffset == textOffset) {
        return 0;
    }
    fprintf(stderr, "marquee {%d,%d,%d} produced {%d,%d,%d,%d}\n",
            first, second, timer, state.firstScroll, state.secondScroll,
            state.brightness, state.textOffset);
    return 1;
}

int main(void) {
    if (Check(240, 120, 0, 236, 116, 0x80, 0)) return 1;
    if (Check(224, 0, 1, 220, -4, 0x40, 40)) return 1;
    if (Check(-620, -624, 2, -624, 240, 0x80, 80)) return 1;
    if (Check(-624, -628, 3, 240, 240, 0x80, 120)) return 1;
    return Check(16, 15, 4, 12, 11, 0x80, 0);
}
