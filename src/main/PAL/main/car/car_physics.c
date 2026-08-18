#include "game/car_physics.h"

s16 CarUpdatePedalLatch(s16 latch, s32 input) {
    if (latch == 0) {
        if (input >= 0x85) return 1;
    } else if (latch == 1) {
        return 2;
    } else if (input < 0x7C) {
        return 0;
    }
    return latch;
}

s32 CarCalculateGripBudget(s32 acceleratorInput, s32 brakeInput) {
    s32 frontLoad = acceleratorInput * 0x64;
    s32 frontLoadScaled = frontLoad >> 8;
    if (frontLoad < 0) frontLoadScaled = (frontLoad + 0xFF) >> 8;
    return 0x17C - frontLoadScaled + (brakeInput * 0x64) / 256;
}
