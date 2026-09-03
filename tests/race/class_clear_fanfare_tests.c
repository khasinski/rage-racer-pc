#include "common.h"
#include "game/menu.h"
#include "game/race.h"

#include <limits.h>
#include <stdio.h>

s32 g_ClassClearFanfareTimer;

static s32 s_cueCount;
static s32 s_lastCue;
static s32 s_failures;

void PlaySoundCue(s32 cue) {
    s_cueCount++;
    s_lastCue = cue;
}

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

int main(void) {
    s32 frame;

    g_ClassClearFanfareTimer = CLASS_CLEAR_FANFARE_DURATION_FRAMES;
    for (frame = 0; frame < 29; frame++) {
        TickClassClearFanfare();
    }
    Check("timer before cue", g_ClassClearFanfareTimer, 181);
    Check("no early cue", s_cueCount, 0);

    TickClassClearFanfare();
    Check("timer at cue", g_ClassClearFanfareTimer, 180);
    Check("one class-clear cue", s_cueCount, 1);
    Check("class-clear cue id", s_lastCue, 0x42);

    while (g_ClassClearFanfareTimer != 0) {
        TickClassClearFanfare();
    }
    Check("cue is not repeated", s_cueCount, 1);
    TickClassClearFanfare();
    Check("zero timer remains zero", g_ClassClearFanfareTimer, 0);
    Check("zero timer has no cue", s_cueCount, 1);

    g_ClassClearFanfareTimer = INT_MIN;
    TickClassClearFanfare();
    Check("negative timer resets", g_ClassClearFanfareTimer, 0);
    Check("negative timer has no cue", s_cueCount, 1);

    return s_failures != 0;
}
