#include "common.h"
#include "game/menu.h"
#include "game/race.h"

#include <stdio.h>

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

    StartClassClearFanfare();
    for (frame = 0; frame < 29; frame++) {
        TickClassClearFanfare();
    }
    Check("no early cue", s_cueCount, 0);

    Check("timer at cue", TickClassClearFanfare(), 180);
    Check("one class-clear cue", s_cueCount, 1);
    Check("class-clear cue id", s_lastCue, 0x42);

    while (TickClassClearFanfare() != 0) {
    }
    Check("cue is not repeated", s_cueCount, 1);
    Check("zero timer remains zero", TickClassClearFanfare(), 0);
    Check("zero timer has no cue", s_cueCount, 1);

    return s_failures != 0;
}
