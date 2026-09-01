#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>

SequenceHandle g_SeqHandle;
s32 g_SeqVolume;

static s16 s_sequence;
static s16 s_leftVolume;
static s16 s_rightVolume;
static s32 s_reverbLeft;
static s32 s_reverbRight;

void SsSeqSetVol(short sequence, short left, short right) {
    s_sequence = sequence;
    s_leftVolume = left;
    s_rightVolume = right;
}

void SetReverbDepth(s32 left, s32 right) {
    s_reverbLeft = left;
    s_reverbRight = right;
}

int main(void) {
    g_SeqHandle.value = 7;
    g_SeqVolume = 0x12348056;

    ApplyCurrentSequenceAudio();
    if (s_sequence != 7 || s_leftVolume != (s16)0x8056 ||
        s_rightVolume != (s16)0x8056 || s_reverbLeft != 0x28 ||
        s_reverbRight != 0x28) {
        puts("current sequence audio setup failed");
        return 1;
    }
    puts("current sequence audio setup preserved");
    return 0;
}
