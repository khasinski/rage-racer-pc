#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdio.h>
#include <string.h>

Cd g_Cd;

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
    CHECK(g_Cd.fade == 0xFFF);
    StartCdVolumeFade(-5000);
    CHECK(g_Cd.fade == -0xFFF);

    g_Cd.mix.ll = 0x40000;
    g_Cd.mix.lr = 0x30000;
    g_Cd.mix.rr = 0x20000;
    g_Cd.mix.rl = 0x10000;
    StartCdVolumeFade(4);
    StepCdVolumeFade();
    CHECK(g_Cd.fade == 3);
    CHECK(g_Cd.mix.ll == 0x30000 && g_Cd.mix.lr == 0x24000);
    CHECK(g_Cd.mix.rr == 0x18000 && g_Cd.mix.rl == 0xC000);
    CHECK(s_lastMix[0] == 0x30 && s_lastMix[1] == 0x24);
    CHECK(s_lastMix[2] == 0x18 && s_lastMix[3] == 0xC);
    StepCdVolumeFade();
    StepCdVolumeFade();
    StepCdVolumeFade();
    CHECK(g_Cd.fade == 0);
    CHECK(g_Cd.mix.ll == 0 && g_Cd.mix.lr == 0 && g_Cd.mix.rr == 0 &&
          g_Cd.mix.rl == 0);

    g_Cd.fullMix.ll = 0x40000;
    g_Cd.fullMix.lr = 0x30000;
    g_Cd.fullMix.rr = 0x20000;
    g_Cd.fullMix.rl = 0x10000;
    StartCdVolumeFade(-4);
    StepCdVolumeFade();
    CHECK(g_Cd.fade == -3);
    CHECK(g_Cd.mix.ll == 0x10000 && g_Cd.mix.lr == 0xC000);
    CHECK(g_Cd.mix.rr == 0x8000 && g_Cd.mix.rl == 0x4000);
    StepCdVolumeFade();
    StepCdVolumeFade();
    StepCdVolumeFade();
    CHECK(g_Cd.fade == 0);
    CHECK(g_Cd.mix.ll == g_Cd.fullMix.ll && g_Cd.mix.lr == g_Cd.fullMix.lr);
    CHECK(g_Cd.mix.rr == g_Cd.fullMix.rr && g_Cd.mix.rl == g_Cd.fullMix.rl);
    CHECK(s_mixCalls == 8);

    g_Cd.mix.ll = 0x40000;
    g_Cd.fullMix.ll = 0x20000;
    StartCdVolumeFade(-2);
    StepCdVolumeFade();
    CHECK(g_Cd.mix.ll == 0x30000 && g_Cd.fade == -1);
    StepCdVolumeFade();
    CHECK(g_Cd.mix.ll == 0x20000 && g_Cd.fade == 0);

    StepCdVolumeFade();
    CHECK(s_mixCalls == 11);
    CHECK(g_Cd.fade == 0);

    puts("CD volume fade tests passed");
    return 0;
}
