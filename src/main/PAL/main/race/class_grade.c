#include "game/save_internal.h"


s32 ComputeClassGrade(void) {
    CourseProgressByteAddress ptr;
    s32 value;
    CourseProgressByteAddress end;
    u8 extra;

    ptr.pointer = g_CourseProgress->bestPlace;
    value = 0;
    if (g_CourseProgress->unlockPending != 0) {
        return 0;
    }

    end.pointer = ptr.pointer + 3;
    do {
        value += *ptr.pointer++;
    } while (ptr.value < end.value);

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
