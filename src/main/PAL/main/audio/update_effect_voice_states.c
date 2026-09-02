#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    EFFECT_VOICE_COUNT = 4,
    EFFECT_HARDWARE_VOICE_FIRST = 10,
    EFFECT_BASE_NOTE = 0x3C,
};

static void KeyOnEffectVoice(s16 hardwareVoice, const EffectVoice *effect) {
    SsUtKeyOnV(hardwareVoice, g_SoundScale.vabIds[0],
               effect->note.half.value, (s16)effect->tone,
               EFFECT_BASE_NOTE, 0, 0, 0);
}

static void WriteEffectVoiceOutput(s16 hardwareVoice,
                                   const EffectVoice *effect) {
    s32 volume = ClampVoiceVolume(
        effect->volume * g_SoundScale.scale / 128);

    SsUtSetVVol(hardwareVoice, volume, volume);
    SsUtChangePitch(
        hardwareVoice, 0, effect->note.half.value, EFFECT_BASE_NOTE, 0,
        (s16)(effect->pitch.value >> 7), effect->pitch.half.fraction & 0x7F);
}

static void ConsumeEffectVoiceUpdate(s16 hardwareVoice, EffectVoice *effect) {
    WriteEffectVoiceOutput(hardwareVoice, effect);
    effect->state = EFFECT_VOICE_IDLE;
}

void ForcePitchEffectVoicesEnabled(s32 enabled) {
    s32 index;

    for (index = 0; index < EFFECT_VOICE_COUNT; index++) {
        EffectVoice *effect = &g_EffectVoices[index];
        s16 hardwareVoice = (s16)(EFFECT_HARDWARE_VOICE_FIRST + index);

        if (enabled != 0) {
            if (effect->note.value < 0) {
                continue;
            }
            KeyOnEffectVoice(hardwareVoice, effect);
            WriteEffectVoiceOutput(hardwareVoice, effect);
        } else {
            SsUtKeyOffV(hardwareVoice);
        }
    }
}

void UpdateEffectVoiceStates(void) {
    s32 index;

    for (index = 0; index < EFFECT_VOICE_COUNT; index++) {
        EffectVoice *effect = &g_EffectVoices[index];
        s16 hardwareVoice = (s16)(EFFECT_HARDWARE_VOICE_FIRST + index);

        switch (effect->state) {
        case EFFECT_VOICE_START:
            KeyOnEffectVoice(hardwareVoice, effect);
            ConsumeEffectVoiceUpdate(hardwareVoice, effect);
            break;
        case EFFECT_VOICE_UPDATE:
            ConsumeEffectVoiceUpdate(hardwareVoice, effect);
            break;
        case EFFECT_VOICE_STOP:
            SsUtKeyOffV(hardwareVoice);
            effect->state = EFFECT_VOICE_IDLE;
            break;
        case EFFECT_VOICE_IDLE:
            break;
        }
    }
}
