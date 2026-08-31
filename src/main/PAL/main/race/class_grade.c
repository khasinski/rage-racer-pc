#include "game/save_internal.h"


s32 ComputeClassGrade(void) {
    s32 value;
    s32 i;
    u8 extra;

    value = 0;
    if (g_CourseProgress->unlockPending != 0) {
        return 0;
    }

    for (i = 0; i < 3; i++) {
        value += g_CourseProgress->bestPlace[i];
    }

    extra = g_CourseProgress->bestPlace[3];
    if (extra == 0xFF) {
        value++;
    } else {
        value += extra;
    }

    value -= 3;
    if (value >= 4) {
        value = 0;
    }
    return value;
}
