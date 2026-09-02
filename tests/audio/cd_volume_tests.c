#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>

u8 g_CdVolume;
s32 g_CdMixPreset;
u8 g_CdMixPresets[8];
u32 g_CdMixLL;
u32 g_CdMixLR;
u32 g_CdMixRR;
u32 g_CdMixRL;
u32 g_CdMixFullLL;
u32 g_CdMixFullLR;
u32 g_CdMixFullRR;
u32 g_CdMixFullRL;

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

    g_CdMixPreset = 0;
    SetCdVolume(127);
    CHECK(g_CdVolume == 127 && s_fadeSteps == 1);
    CHECK(g_CdMixLL == 127U * 4096 && g_CdMixLR == 64U * 4096);
    CHECK(g_CdMixRR == 32U * 4096 && g_CdMixRL == 0);
    CHECK(g_CdMixLL == g_CdMixFullLL && g_CdMixLR == g_CdMixFullLR);
    CHECK(g_CdMixRR == g_CdMixFullRR && g_CdMixRL == g_CdMixFullRL);

    g_CdMixPreset = 1;
    SetCdVolume(63);
    CHECK(g_CdMixLL == (10 * 63 / 127) * 4096U);
    CHECK(g_CdMixLR == (20 * 63 / 127) * 4096U);
    CHECK(g_CdMixRR == (30 * 63 / 127) * 4096U);
    CHECK(g_CdMixRL == (40 * 63 / 127) * 4096U);

    g_CdMixPreset = 0;
    SetCdVolumeSetting(0);
    CHECK(g_CdVolume == 0 && g_CdMixLL == 0);
    SetCdVolumeSetting(15);
    CHECK(g_CdVolume == 127 && g_CdMixLL == 127U * 4096);
    SetCdVolumeSetting(8);
    CHECK(g_CdVolume == 67);
    SetCdVolume(-20);
    CHECK(g_CdVolume == 0);
    SetCdVolume(300);
    CHECK(g_CdVolume == 127);
    SetCdVolumeSetting(-1);
    CHECK(g_CdVolume == 0);
    SetCdVolumeSetting(20);
    CHECK(g_CdVolume == 127);

    SetCdMixPreset(-1);
    CHECK(g_CdMixPreset == 0 && g_CdMixLL == 127U * 4096);
    SetCdMixPreset(9);
    CHECK(g_CdMixPreset == 1 && g_CdMixLL == 10U * 4096);
    CHECK(s_fadeSteps == 11);

    puts("CD volume tests passed");
    return 0;
}
