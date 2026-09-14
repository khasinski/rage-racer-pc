#include "game/race_internal.h"

enum { BGM_SELECT_TIMER_LIMIT = 10000 };

s32 NextBgmSelectTimer(s32 timer) {
    if (timer < 0) {
        return 0;
    }
    return timer < BGM_SELECT_TIMER_LIMIT ? timer + 1
                                          : BGM_SELECT_TIMER_LIMIT;
}
