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

static s32 ApplyFadeOutStep(s32 value, s32 step) {
    int64_t next = (int64_t)value + step;

    return next > 0 ? (s32)next : 0;
}

static void UpdateReverbFade(void) {
    s32 delta = g_ReverbFadeStep;

    if (delta >= 0) {
        g_ReverbFadeStep = 0;
        return;
    }
    g_ReverbDepthL = ApplyFadeOutStep(g_ReverbDepthL, delta);
    g_ReverbDepthR = ApplyFadeOutStep(g_ReverbDepthR, delta);

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
    if (g_SeqVolumeFadeStep > 0) {
        g_SeqVolumeFadeStep = 0;
    }
    UpdateReverbFade();

    SetReverbDepth(g_ReverbDepthL, g_ReverbDepthR);

    if (g_SeqVolumeFadeStep == 0) {
        SetSequenceVolume(g_SeqVolume);
        return;
    }

    g_SeqVolume = ApplyFadeOutStep(g_SeqVolume, g_SeqVolumeFadeStep);
    if (g_SeqVolume <= 0) {
        FinishSequenceFadeOut();
        return;
    }

    SetSequenceVolume(g_SeqVolume);
}

void ApplyDuckedSequenceAudio(void) {
    s32 volume = ClampVoiceVolume(g_SeqVolume) * DUCKED_VOLUME_NUMERATOR /
                 DUCKED_VOLUME_DENOMINATOR;

    SsSeqSetVol(g_SeqHandle.value, volume, volume);
    SetReverbDepth(DUCKED_REVERB_DEPTH, DUCKED_REVERB_DEPTH);
}

void ApplyCurrentSequenceAudio(void) {
    s16 volume = (s16)ClampVoiceVolume(g_SeqVolume);

    SsSeqSetVol(g_SeqHandle.value, volume, volume);
    SetDefaultReverbDepth();
}
