#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_CdFadeFrames;
u32 g_CdMixLL;
u32 g_CdMixLR;
u32 g_CdMixRR;
u32 g_CdMixRL;
u32 g_CdMixFullLL;
u32 g_CdMixFullLR;
u32 g_CdMixFullRR;
u32 g_CdMixFullRL;

static u8 s_lastMix[4];
static s32 s_mixCalls;

void CdMix(u8 *mix) {
    memcpy(s_lastMix, mix, sizeof(s_lastMix));
    s_mixCalls++;
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
    StartCdVolumeFade(5000);
    CHECK(g_CdFadeFrames == 0xFFF);
    StartCdVolumeFade(-5000);
    CHECK(g_CdFadeFrames == -0xFFF);

    g_CdMixLL = 0x40000;
    g_CdMixLR = 0x30000;
    g_CdMixRR = 0x20000;
    g_CdMixRL = 0x10000;
    StartCdVolumeFade(4);
    StepCdVolumeFade();
    CHECK(g_CdFadeFrames == 3);
    CHECK(g_CdMixLL == 0x30000 && g_CdMixLR == 0x24000);
    CHECK(g_CdMixRR == 0x18000 && g_CdMixRL == 0xC000);
    CHECK(s_lastMix[0] == 0x30 && s_lastMix[1] == 0x24);
    CHECK(s_lastMix[2] == 0x18 && s_lastMix[3] == 0xC);
    StepCdVolumeFade();
    StepCdVolumeFade();
    StepCdVolumeFade();
    CHECK(g_CdFadeFrames == 0);
    CHECK(g_CdMixLL == 0 && g_CdMixLR == 0 && g_CdMixRR == 0 &&
          g_CdMixRL == 0);

    g_CdMixFullLL = 0x40000;
    g_CdMixFullLR = 0x30000;
    g_CdMixFullRR = 0x20000;
    g_CdMixFullRL = 0x10000;
    StartCdVolumeFade(-4);
    StepCdVolumeFade();
    CHECK(g_CdFadeFrames == -3);
    CHECK(g_CdMixLL == 0x10000 && g_CdMixLR == 0xC000);
    CHECK(g_CdMixRR == 0x8000 && g_CdMixRL == 0x4000);
    StepCdVolumeFade();
    StepCdVolumeFade();
    StepCdVolumeFade();
    CHECK(g_CdFadeFrames == 0);
    CHECK(g_CdMixLL == g_CdMixFullLL && g_CdMixLR == g_CdMixFullLR);
    CHECK(g_CdMixRR == g_CdMixFullRR && g_CdMixRL == g_CdMixFullRL);
    CHECK(s_mixCalls == 8);

    g_CdMixLL = 0x40000;
    g_CdMixFullLL = 0x20000;
    StartCdVolumeFade(-2);
    StepCdVolumeFade();
    CHECK(g_CdMixLL == 0x30000 && g_CdFadeFrames == -1);
    StepCdVolumeFade();
    CHECK(g_CdMixLL == 0x20000 && g_CdFadeFrames == 0);

    StepCdVolumeFade();
    CHECK(s_mixCalls == 11);
    CHECK(g_CdFadeFrames == 0);

    puts("CD volume fade tests passed");
    return 0;
}
