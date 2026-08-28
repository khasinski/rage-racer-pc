#include "game/cd.h"
#include "game/cd_internal.h"

void StartCdVolumeFade(s32 frames) {
    g_CdFadeFrames = frames;
    if (frames >= 0x1000) {
        g_CdFadeFrames = 0xFFF;
    }
    if (g_CdFadeFrames < -0xFFF) {
        g_CdFadeFrames = -0xFFF;
    }
}

/* The live CdlATV mixer, in 12.12 fixed point: the four values are shifted
 * right 12 to make the 0..0x7F bytes CdMix wants, so 0x7F000 is full. The
 * matching g_CdMixFull* four at +0x10 are the target the fade ramps toward.
 * Channel order is CdlATV's: L->L, L->R, R->R, R->L. */

void StepCdVolumeFade(void) {
    u8 buf[4];
    s32 cnt;

    cnt = g_CdFadeFrames;
    if (cnt > 0) {
        u32 *p;
        s32 n;
        u32 q1;
        u32 q2;
        u32 q3;
        u32 q4;

        p = &g_CdMixLL;
        n = cnt - 1;
        q1 = (*p * n) / cnt;
        q2 = (g_CdMixLR * n) / cnt;
        q3 = (g_CdMixRR * n) / cnt;
        q4 = (g_CdMixRL * n) / cnt;
        g_CdFadeFrames = n;
        *p = q1;
        g_CdMixLR = q2;
        g_CdMixRR = q3;
        g_CdMixRL = q4;
    } else if (cnt < 0) {
        u32 *p;
        u32 v184;
        u32 inv;
        u32 div2;
        u32 d1;
        u32 v188;
        u32 d2;
        u32 v18C;
        u32 d3;
        u32 v190;
        u32 d4;
        s32 c2;

        p = &g_CdMixLL;
        v184 = g_CdMixFullLL;
        inv = ~cnt;
        div2 = inv + 1;
        d1 = ((v184 - *p) * inv) / div2;
        v188 = g_CdMixFullLR;
        d2 = ((v188 - g_CdMixLR) * inv) / div2;
        v18C = g_CdMixFullRR;
        d3 = ((v18C - g_CdMixRR) * inv) / div2;
        v190 = g_CdMixFullRL;
        d4 = ((v190 - g_CdMixRL) * inv) / div2;
        v184 = v184 - d1;
        *p = v184;
        c2 = cnt + 1;
        g_CdFadeFrames = c2;
        v188 = v188 - d2;
        g_CdMixLR = v188;
        v18C = v18C - d3;
        g_CdMixRR = v18C;
        v190 = v190 - d4;
        g_CdMixRL = v190;
    }
    buf[0] = g_CdMixLL / 4096;
    buf[1] = g_CdMixLR / 4096;
    buf[2] = g_CdMixRR / 4096;
    buf[3] = g_CdMixRL / 4096;
    CdMix(buf);
}
