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

static u32 FadeChannelOut(u32 level, s32 framesRemaining) {
    return level * (framesRemaining - 1) / framesRemaining;
}

static u32 FadeChannelIn(u32 level, u32 target, s32 framesRemaining) {
    return target -
           (target - level) * (framesRemaining - 1) / framesRemaining;
}

void StepCdVolumeFade(void) {
    u8 mix[4];
    s32 framesRemaining = g_CdFadeFrames;

    if (framesRemaining > 0) {
        g_CdMixLL = FadeChannelOut(g_CdMixLL, framesRemaining);
        g_CdMixLR = FadeChannelOut(g_CdMixLR, framesRemaining);
        g_CdMixRR = FadeChannelOut(g_CdMixRR, framesRemaining);
        g_CdMixRL = FadeChannelOut(g_CdMixRL, framesRemaining);
        g_CdFadeFrames--;
    } else if (framesRemaining < 0) {
        framesRemaining = -framesRemaining;
        g_CdMixLL =
            FadeChannelIn(g_CdMixLL, g_CdMixFullLL, framesRemaining);
        g_CdMixLR =
            FadeChannelIn(g_CdMixLR, g_CdMixFullLR, framesRemaining);
        g_CdMixRR =
            FadeChannelIn(g_CdMixRR, g_CdMixFullRR, framesRemaining);
        g_CdMixRL =
            FadeChannelIn(g_CdMixRL, g_CdMixFullRL, framesRemaining);
        g_CdFadeFrames++;
    }

    mix[0] = g_CdMixLL / 4096;
    mix[1] = g_CdMixLR / 4096;
    mix[2] = g_CdMixRR / 4096;
    mix[3] = g_CdMixRL / 4096;
    CdMix(mix);
}
