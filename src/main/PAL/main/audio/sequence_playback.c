#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    SEQUENCE_PLAY_MODE = 1,
    SEQUENCE_LOOP_COUNT = 0,
    SEQUENCE_VOLUME_FADE_STEP = -4,
    REVERB_VOLUME_FADE_STEP = -3,
    DUCKED_VOLUME_NUMERATOR = 3,
    DUCKED_VOLUME_DENOMINATOR = 4,
    DUCKED_REVERB_DEPTH = 0x3C,
};

void PlaySequence(void) {
    SsSeqPlay(g_SeqHandle.value, SEQUENCE_PLAY_MODE, SEQUENCE_LOOP_COUNT);
}

void StartSequenceFadeOut(void) {
    g_SeqVolumeFadeStep = SEQUENCE_VOLUME_FADE_STEP;
    g_ReverbFadeStep = REVERB_VOLUME_FADE_STEP;
}

static void UpdateReverbFade(void) {
    s32 delta = g_ReverbFadeStep;

    if (delta == 0) return;
    g_ReverbDepthL += delta;
    if (g_ReverbDepthL < 0) {
        g_ReverbDepthL = 0;
    }

    g_ReverbDepthR += delta;
    if (g_ReverbDepthR < 0) {
        g_ReverbDepthR = 0;
    }

    if (g_ReverbDepthL == 0 && g_ReverbDepthR == 0) {
        g_ReverbFadeStep = 0;
    }
}

static void FinishSequenceFadeOut(void) {
    g_SeqVolume = 0;
    g_SeqVolumeFadeStep = 0;
    SsSeqStop(g_SeqHandle.value);
    CloseSequenceAudioSlot();
    SetDefaultReverbDepth();
}

void UpdateSequenceFadeOut(void) {
    UpdateReverbFade();

    SetReverbDepth(g_ReverbDepthL, g_ReverbDepthR);

    g_SeqVolume += g_SeqVolumeFadeStep;
    if (g_SeqVolume <= 0) {
        FinishSequenceFadeOut();
        return;
    }

    SetSequenceVolume(g_SeqVolume);
}

void ApplyDuckedSequenceAudio(void) {
    s32 volume = g_SeqVolume * DUCKED_VOLUME_NUMERATOR /
                 DUCKED_VOLUME_DENOMINATOR;

    SsSeqSetVol(g_SeqHandle.value, volume, volume);
    SetReverbDepth(DUCKED_REVERB_DEPTH, DUCKED_REVERB_DEPTH);
}

void ApplyCurrentSequenceAudio(void) {
    s16 volume = (s16)g_SeqVolume;

    SsSeqSetVol(g_SeqHandle.value, volume, volume);
    SetDefaultReverbDepth();
}
