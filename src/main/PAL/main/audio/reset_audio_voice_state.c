#include "game/audio_state_internal.h"
#include "game/audio.h"
#include "game/sound.h"

enum {
    DEFAULT_EFFECT_PITCH = 0x1E00,
};

void ResetAudioVoiceState(void) {
    s32 i;

    for (i = 0; i < AUDIO_MUSIC_CHANNEL_COUNT; i++) {
        g_MusicChannels[i].mode = MUSIC_CHANNEL_IDLE;
        g_MusicChannels[i].left.value = -1;
        g_MusicChannels[i].right.value = -1;
        g_MusicChannels[i].volLeft = 0;
        g_MusicChannels[i].volRight = 0;
    }

    for (i = 0; i < AUDIO_EFFECT_VOICE_COUNT; i++) {
        g_EffectVoices[i].state = EFFECT_VOICE_IDLE;
        g_EffectVoices[i].note.value = -1;
        g_EffectVoices[i].tone = -1;
        g_EffectVoices[i].pitch.value = DEFAULT_EFFECT_PITCH;
        g_EffectVoices[i].volume = 0;
    }

    g_PanVoiceVolumeR = -1;
    g_PanVoiceVolumeL = -1;
    g_IndexedEffectIndexPrev = -1;
    g_IndexedEffectIndex = -1;
    g_PanVoiceActive = 0;
    g_IndexedEffectPitch = DEFAULT_EFFECT_PITCH;
    g_IndexedEffectVolume = 0;
    g_ActiveSpecialCue = -1;
    g_LastSpecialCueRequest = -1;
}
