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

    g_Audio.pan = (PanVoice){.left = -1, .right = -1};
    g_Audio.indexed = (IndexedVoice){
        .index = -1,
        .previous = -1,
        .pitch = DEFAULT_EFFECT_PITCH,
    };
    g_Audio.cue = (SpecialCue){.active = -1, .previous = -1};
}
