#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    EFFECT_VOICE_COUNT = 4,
    EFFECT_HARDWARE_VOICE_FIRST = 10,
    EFFECT_BASE_NOTE = 0x3C,
};

static void ApplyEffectVoicePitch(s32 hardwareVoice, EffectVoice *effect) {
    s32 volume = ClampVoiceVolume(
        effect->volume * g_SoundScale.scale / 128);

    SsUtSetVVol((s16)hardwareVoice, volume, volume);
    SsUtChangePitch(
        (s16)hardwareVoice, 0, effect->note.half.value, EFFECT_BASE_NOTE, 0,
        (s16)(effect->pitch.value >> 7), effect->pitch.half.fraction & 0x7F);
    effect->state = -1;
}

void UpdateEffectVoiceStates(void) {
    s32 index;

    for (index = 0; index < EFFECT_VOICE_COUNT; index++) {
        EffectVoice *effect = &g_EffectVoices[index];
        s32 hardwareVoice = EFFECT_HARDWARE_VOICE_FIRST + index;

        switch (effect->state) {
        case 0:
            SsUtKeyOnV((s16)hardwareVoice, g_SoundScale.vabIds[0],
                       effect->note.half.value, (s16)effect->tone,
                       EFFECT_BASE_NOTE, 0, 0, 0);
            ApplyEffectVoicePitch(hardwareVoice, effect);
            break;
        case 2:
            ApplyEffectVoicePitch(hardwareVoice, effect);
            break;
        case 1:
            SsUtKeyOffV((s16)hardwareVoice);
            effect->state = -1;
            break;
        }
    }
}
