#include "game/audio.h"
#include "game/sound.h"

void PlaySequence(void) { SsSeqPlay(g_SeqHandle.value, 1, 0); }

void StopSequence(void) { SsSeqStop(g_SeqHandle.value); }

void StartSequenceFadeOut(void) {
    g_SeqVolumeFadeStep = -4;
    g_ReverbFadeStep = -3;
}


void UpdateSequenceFadeOut(void) {
    s32 delta = g_ReverbFadeStep;

    if (delta != 0) {
        g_ReverbDepthL += delta;
        if (g_ReverbDepthL < 0) {
            g_ReverbDepthL = 0;
        }

        g_ReverbDepthR += delta;
        if (g_ReverbDepthR < 0) {
            g_ReverbDepthR = 0;
        }

        if ((g_ReverbDepthL == 0) && (g_ReverbDepthR == 0)) {
            g_ReverbFadeStep = 0;
        }
    }

    SetReverbDepth(g_ReverbDepthL, g_ReverbDepthR);

    g_SeqVolume += g_SeqVolumeFadeStep;
    if (g_SeqVolume <= 0) {
        g_SeqVolume = 0;
        g_SeqVolumeFadeStep = 0;
        StopSequence();
        CloseAudioSlot(6);
        SetReverbDepth(0x28, 0x28);
    }

    SetSequenceVolume(g_SeqVolume);
}

void ApplyDuckedSequenceAudio(void) {
    s32 volume = g_SeqVolume * 3 / 4;

    SsSeqSetVol(g_SeqHandle.value, volume, volume);
    SetReverbDepth(0x3C, 0x3C);
}
