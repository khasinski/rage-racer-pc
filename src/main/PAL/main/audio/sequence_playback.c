#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

void PlaySequence(void) { SsSeqPlay(g_SeqHandle.value, 1, 0); }

void StopSequence(void) { SsSeqStop(g_SeqHandle.value); }

void StartSequenceFadeOut(void) {
    g_SeqVolumeFadeStep = -4;
    g_ReverbFadeStep = -3;
}


void UpdateSequenceFadeOut(void) {
    s32 delta;
    s32 value;
    s32 depthLeft;
    s32 depthRight;

    delta = g_ReverbFadeStep;
    if (delta != 0) {
        value = g_ReverbDepthL;
        value += delta;
        if ((g_ReverbDepthL = value) < 0) {
            g_ReverbDepthL = 0;
        }

        value = g_ReverbDepthR;
        value += delta;
        g_ReverbDepthR = value;
        if (value < 0) {
            g_ReverbDepthR = 0;
        }

        if ((g_ReverbDepthL == 0) && (g_ReverbDepthR == 0)) {
            g_ReverbFadeStep = 0;
        }
    }

    SetReverbDepth(g_ReverbDepthL, g_ReverbDepthR);

    value = g_SeqVolume;
    delta = g_SeqVolumeFadeStep;
    value += delta;
    g_SeqVolume = value;
    if (value <= 0) {
        g_SeqVolume = 0;
        g_SeqVolumeFadeStep = 0;
        StopSequence();
        CloseAudioSlot(6);
        depthLeft = 0x28;
        depthRight = 0x28;
        SetReverbDepth(depthLeft, depthRight);
    }

    SetSequenceVolume(g_SeqVolume);
}

void ApplyDuckedSequenceAudio(void) {
    s32 value;
    s32 scaled;
    s32 seq;
    s32 volume;

    value = g_SeqVolume;
    seq = g_SeqHandle.value;
    scaled = value * 2;
    {
        s32 rel = value;
        value = scaled + rel;
    }
    scaled = value;
    if (value < 0) {
        scaled = value + 3;
    }
    scaled <<= 0xE;
    volume = scaled >> 0x10;
    SsSeqSetVol(seq, volume, volume);
    SetReverbDepth(0x3C, 0x3C);
}
