#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"
#include <psyz/audio.h>

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
    if (Psyz_PcmMusicIsLoaded())
        Psyz_PcmMusicPlay(1);
    else
        SsSeqPlay((s16)g_Audio.seq.handle, SEQUENCE_PLAY_MODE,
                  SEQUENCE_LOOP_COUNT);
}

void StartSequenceFadeOut(void) {
    g_Audio.seq.fade = SEQUENCE_VOLUME_FADE_STEP;
    g_Audio.reverb.fade = REVERB_VOLUME_FADE_STEP;
}

static s32 ApplyFadeOutStep(s32 value, s32 step) {
    int64_t next = (int64_t)value + step;

    return next > 0 ? (s32)next : 0;
}

static void UpdateReverbFade(void) {
    s32 delta = g_Audio.reverb.fade;

    if (delta >= 0) {
        g_Audio.reverb.fade = 0;
        return;
    }
    g_Audio.reverb.left = ApplyFadeOutStep(g_Audio.reverb.left, delta);
    g_Audio.reverb.right = ApplyFadeOutStep(g_Audio.reverb.right, delta);

    if (g_Audio.reverb.left == 0 && g_Audio.reverb.right == 0) {
        g_Audio.reverb.fade = 0;
    }
}

static void FinishSequenceFadeOut(void) {
    g_Audio.seq.volume = 0;
    g_Audio.seq.fade = 0;
    SsSeqStop((s16)g_Audio.seq.handle);
    Psyz_PcmMusicStop();
    CloseSequenceAudioSlot();
    SetDefaultReverbDepth();
}

void UpdateSequenceFadeOut(void) {
    if (g_Audio.seq.fade > 0) {
        g_Audio.seq.fade = 0;
    }
    UpdateReverbFade();

    SetReverbDepth(g_Audio.reverb.left, g_Audio.reverb.right);

    if (g_Audio.seq.fade == 0) {
        SetSequenceVolume(g_Audio.seq.volume);
        return;
    }

    g_Audio.seq.volume = ApplyFadeOutStep(g_Audio.seq.volume, g_Audio.seq.fade);
    if (g_Audio.seq.volume <= 0) {
        FinishSequenceFadeOut();
        return;
    }

    SetSequenceVolume(g_Audio.seq.volume);
}

void ApplyDuckedSequenceAudio(void) {
    s32 volume = ClampVoiceVolume(g_Audio.seq.volume) * DUCKED_VOLUME_NUMERATOR /
                 DUCKED_VOLUME_DENOMINATOR;

    SsSeqSetVol((s16)g_Audio.seq.handle, volume, volume);
    Psyz_PcmMusicSetVolume(volume);
    SetReverbDepth(DUCKED_REVERB_DEPTH, DUCKED_REVERB_DEPTH);
}

void ApplyCurrentSequenceAudio(void) {
    s16 volume = (s16)ClampVoiceVolume(g_Audio.seq.volume);

    SsSeqSetVol((s16)g_Audio.seq.handle, volume, volume);
    SetDefaultReverbDepth();
}
