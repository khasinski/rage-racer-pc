#include "game/cd.h"
#include "game/cd_internal.h"

#include <stdint.h>

enum { CD_FADE_FRAME_LIMIT = 0xFFF };

void StartCdVolumeFade(s32 frames) {
    g_Cd.fade = frames;
    if (frames > CD_FADE_FRAME_LIMIT) {
        g_Cd.fade = CD_FADE_FRAME_LIMIT;
    }
    if (g_Cd.fade < -CD_FADE_FRAME_LIMIT) {
        g_Cd.fade = -CD_FADE_FRAME_LIMIT;
    }
}

/* The live CdlATV mixer, in 12-bit fixed point: the four values are divided
 * by CD_MIX_FIXED_ONE to make the 0..0x7F bytes CdMix wants, so 0x7F000 is full. The
 * matching g_CdMixFull* four at +0x10 are the target the fade ramps toward.
 * Channel order is CdlATV's: L->L, L->R, R->R, R->L. */

static u32 FadeChannelOut(u32 level, s32 framesRemaining) {
    return level * (framesRemaining - 1) / framesRemaining;
}

static u32 FadeChannelIn(u32 level, u32 target, s32 framesRemaining) {
    int64_t difference = (int64_t)target - level;

    return (u32)((int64_t)level + difference / framesRemaining);
}

void StepCdVolumeFade(void) {
    u8 mix[4];
    s32 framesRemaining = g_Cd.fade;

    if (framesRemaining > 0) {
        g_Cd.mix.ll = FadeChannelOut(g_Cd.mix.ll, framesRemaining);
        g_Cd.mix.lr = FadeChannelOut(g_Cd.mix.lr, framesRemaining);
        g_Cd.mix.rr = FadeChannelOut(g_Cd.mix.rr, framesRemaining);
        g_Cd.mix.rl = FadeChannelOut(g_Cd.mix.rl, framesRemaining);
        g_Cd.fade--;
    } else if (framesRemaining < 0) {
        framesRemaining = -framesRemaining;
        g_Cd.mix.ll =
            FadeChannelIn(g_Cd.mix.ll, g_Cd.fullMix.ll, framesRemaining);
        g_Cd.mix.lr =
            FadeChannelIn(g_Cd.mix.lr, g_Cd.fullMix.lr, framesRemaining);
        g_Cd.mix.rr =
            FadeChannelIn(g_Cd.mix.rr, g_Cd.fullMix.rr, framesRemaining);
        g_Cd.mix.rl =
            FadeChannelIn(g_Cd.mix.rl, g_Cd.fullMix.rl, framesRemaining);
        g_Cd.fade++;
    }

    mix[0] = g_Cd.mix.ll / CD_MIX_FIXED_ONE;
    mix[1] = g_Cd.mix.lr / CD_MIX_FIXED_ONE;
    mix[2] = g_Cd.mix.rr / CD_MIX_FIXED_ONE;
    mix[3] = g_Cd.mix.rl / CD_MIX_FIXED_ONE;
    CdMix(mix);
}
