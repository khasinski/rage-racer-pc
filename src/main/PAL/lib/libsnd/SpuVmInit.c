#include <sys/types.h>

#include "common.h"
#include "psyq/snd.h"
#include "game/audio.h"

#define SND_CURRENT_VOICE_QUALIFIER volatile
#define SND_KEY_ON_QUALIFIER volatile
#define SND_KEY_OFF_QUALIFIER volatile
#define SND_VOICE_COUNT_QUALIFIER volatile
#include "psyq/snd_internal.h"
#include "psyq/spu_internal.h"


void SpuVmInit(long voices) {
    s16 i;
    long ff;
    long one;
    long index;
    long offset;
    s16 shifted;
    long eighteen;
    long mindex;
    u_long lowMask;
    u_long highMask;
    register u_long lowBits asm("$3");
    register u_long highBits asm("$4");
    volatile u_short *spu;
    u_long bits;
    u_long n;
    u_long cond;

    {
        u_char *p = g_SpuMallocArea;
        _spu_setTransferCompletionFlag(0);
        D_801E4B5C = 0;
        g_SndDamper = 0;
        asm volatile("" ::);
        SpuInitMalloc(0x20, p);
    }

    for (i = 0; (u_short)i < 192; i++) ((u_short *)g_SndVoiceRegs)[(u_short)i] = 0;
    for (i = 0; (u_short)i < 24; i++) g_SndVoiceFlags[(u_short)i] = 0;
    g_SndVabOpenCount = 0;
    for (i = 0; (u_short)i < 16; i++) g_SndVabStatus[(u_short)i] = 0;

    n = (u8)voices;
    if (n >= 24) {
        g_SndVoiceCount = 24;
    } else {
        g_SndVoiceCount = n;
    }
    if (g_SndVoiceCount != 0) {
            i = 0;
            ff = 0xFF;
            one = 1;
            do {
                index = (u_short)i;
                shifted = index * 8;
                offset = index * 0x34;
                eighteen = 0x18;   *(short *)&((u_char *)g_SndVoiceState + 2)[offset] = eighteen;
                eighteen = -1;     *(short *)&((u_char *)g_SndVoiceState + 14)[offset] = eighteen;
                *(short *)((u_char *)g_SndVoiceState + offset) = ff;
                ((u_char *)g_SndVoiceState + 27)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 4)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 6)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 16)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 18)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 20)[offset] = ff;
                *(short *)&((u_char *)g_SndVoiceState + 8)[offset] = 0;
                eighteen = 0x40;   ((u_char *)g_SndVoiceState + 10)[offset] = eighteen;
                *(short *)&((u_char *)g_SndVoiceState + 28)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 30)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 32)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 34)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 40)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 42)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 44)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 46)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 48)[offset] = 0;
                *(short *)&((u_char *)g_SndVoiceState + 36)[offset] = 0;

                spu = (volatile u_short *)&((u_short *)g_SndSpuRegs)[(u_short)shifted];
                spu[3] = 0x200;
                spu[2] = 0x1000;
                spu[4] = 0x80FF;
                spu[0] = 0;
                spu[1] = 0;
                spu[5] = 0x4000;

                g_SndCurrentVoice = i;
                lowBits = (u_short)g_SndCurrentVoice;
                /* This barrier is load-bearing: it hides the halfword load,
                 * whose known-zero high bits would otherwise let gcc drop the
                 * mask. */
                mindex = lowBits & 0xFFFF;
                if ((u_long)mindex < 0x10) {
                    lowMask = one << mindex;
                    highMask = 0;
                } else {
                    lowMask = 0;
                    offset = mindex - 0x10;
                    highMask = one << offset;
                }

                lowBits &= 0xFFFF;
                offset = lowBits * 0x34;
                ((u_char *)g_SndVoiceState + 27)[offset] = 0;
                lowBits = g_SndKeyOffLow;
                highBits = g_SndKeyOffHigh;
                i++;
                *(short *)&((u_char *)g_SndVoiceState + 4)[offset] = 0;
                *(short *)((u_char *)g_SndVoiceState + offset) = 0;

                bits = g_SndKeyOnLow;
                __asm__ volatile("");
                /* These barriers are load-bearing. Without them `combine` folds
                 * the single-use `zero_extend(mem)` that defines the second
                 * operand into the `ior`, tripping its "complex expression
                 * first" rule and swapping the operands; retail keeps the
                 * written mask-first order. */
                asm("" : "=r"(lowBits) : "0"(lowBits));
                lowBits = lowMask | lowBits;
                asm("" : "=r"(highBits) : "0"(highBits));
                highBits = highMask | highBits;
                g_SndKeyOffLow = lowBits;
                bits = bits & ~lowBits;
                g_SndKeyOffHigh = highBits;
                g_SndKeyOnLow = bits;
                bits = g_SndKeyOnHigh;
                cond = g_SndVoiceCount;
                g_SndKeyOnHigh = bits & ~highBits;
            } while ((u_short)i < cond);
    }

    g_SndReverbAttr.depth.left = 0x3FFF;
    g_SndReverbAttr.depth.right = 0x3FFF;
    g_SndKeyOnLow = 0;
    g_SndKeyOnHigh = 0;
    g_SndKeyOffLow = 0;
    g_SndReverbOnLow = 0;
    g_SndReverbOnHigh = 0;
    g_SndReverbAttr.mask = 0;
    g_SndReverbAttr.mode = 0;
    g_SndReservedVoiceCount = 0;
    g_SndMonoMode = 0;
    g_SndVabProgMax = 0x80;
    SsUtFlush();
}
