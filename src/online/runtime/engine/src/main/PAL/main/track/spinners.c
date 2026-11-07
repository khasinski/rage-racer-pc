#include "game/spinners.h"
#include "game/angle.h"
#include "game/random.h"

void TickSpinners(Spinners *spinners, int multiple, u32 timer, u32 seed, int animate) {
    if (!spinners) return;
    const unsigned first = multiple ? 1 : 0, limit = multiple ? 4 : 1;
    for (unsigned i = first; i < limit; ++i) {
        u32 angle = (u16)spinners->angles[i];
        if (animate) angle += spinners->rates[multiple != 0];
        spinners->angles[i] = (s16)(angle & ANGLE_MASK);
    }
    if (animate && (timer & 0x1ffu) == 0) {
        u32 random = seed ^ timer;
        spinners->rates[0] = (u16)(RandomNext(&random) & 0x1f);
        spinners->rates[1] = (u16)(RandomNext(&random) & 0x3f);
    }
}
