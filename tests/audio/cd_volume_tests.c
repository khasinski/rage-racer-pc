#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>

Cd g_Cd;
u8 g_CdMixPresets[8];

static s32 s_fadeSteps;

void StepCdVolumeFade(void) {
    s_fadeSteps++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_CdMixPresets[0] = 127;
    g_CdMixPresets[1] = 64;
    g_CdMixPresets[2] = 32;
    g_CdMixPresets[3] = 0;
    g_CdMixPresets[4] = 10;
    g_CdMixPresets[5] = 20;
    g_CdMixPresets[6] = 30;
    g_CdMixPresets[7] = 40;

    g_Cd.reservedMixPreset = 0;
    SetCdVolume(127);
    CHECK(g_Cd.volume == 127 && s_fadeSteps == 1);
    CHECK(g_Cd.mix.ll == 127U * 4096 && g_Cd.mix.lr == 64U * 4096);
    CHECK(g_Cd.mix.rr == 32U * 4096 && g_Cd.mix.rl == 0);
    CHECK(g_Cd.mix.ll == g_Cd.fullMix.ll && g_Cd.mix.lr == g_Cd.fullMix.lr);
    CHECK(g_Cd.mix.rr == g_Cd.fullMix.rr && g_Cd.mix.rl == g_Cd.fullMix.rl);

    g_Cd.reservedMixPreset = 1;
    SetCdVolume(63);
    CHECK(g_Cd.mix.ll == (127 * 63 / 127) * 4096U);
    CHECK(g_Cd.mix.lr == (64 * 63 / 127) * 4096U);
    CHECK(g_Cd.mix.rr == (32 * 63 / 127) * 4096U);
    CHECK(g_Cd.mix.rl == 0);

    g_Cd.reservedMixPreset = 0;
    SetCdVolumeSetting(0);
    CHECK(g_Cd.volume == 0 && g_Cd.mix.ll == 0);
    SetCdVolumeSetting(15);
    CHECK(g_Cd.volume == 127 && g_Cd.mix.ll == 127U * 4096);
    SetCdVolumeSetting(8);
    CHECK(g_Cd.volume == 67);
    SetCdVolume(-20);
    CHECK(g_Cd.volume == 0);
    SetCdVolume(300);
    CHECK(g_Cd.volume == 127);
    SetCdVolumeSetting(-1);
    CHECK(g_Cd.volume == 0);
    SetCdVolumeSetting(20);
    CHECK(g_Cd.volume == 127);

    CHECK(s_fadeSteps == 9);

    puts("CD volume tests passed");
    return 0;
}
