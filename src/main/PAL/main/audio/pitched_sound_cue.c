#include "game/audio.h"
#include "game/sound.h"

enum {
    EFFECT_CUE_VOICE_COUNT = 2,
    EFFECT_CUE_SECONDARY_VOICE = 2,
    EFFECT_CUE_DEFAULT_PITCH = 0x1E00,
};

static s32 CueVoiceCount(const EffectCueBank *cue) {
    if (cue->voiceCount < 0) {
        return 0;
    }
    return cue->voiceCount > EFFECT_CUE_VOICE_COUNT
               ? EFFECT_CUE_VOICE_COUNT
               : cue->voiceCount;
}

static s32 CueBankVoiceStart(s32 bank) {
    return bank == 0 ? 0 : EFFECT_CUE_SECONDARY_VOICE;
}

static s32 VoicePairMatchesBank(s32 voiceStart, s32 bank) {
    return g_EffectVoices[voiceStart].note.value ==
               g_EffectCueTable[bank].programs[0].note &&
           g_EffectVoices[voiceStart + 1].note.value ==
               g_EffectCueTable[bank].programs[1].note;
}

static void ResetCueVoices(s32 voiceStart, s32 voiceCount) {
    s32 voice;

    for (voice = 0; voice < voiceCount; voice++) {
        EffectVoice *effect = &g_EffectVoices[voiceStart + voice];

        effect->state = EFFECT_VOICE_STOP;
        effect->note.value = -1;
        effect->tone = -1;
        effect->pitch.value = EFFECT_CUE_DEFAULT_PITCH;
        effect->volume = 0;
    }
}

static void StartCueVoices(s32 voiceStart, s32 bank, s32 pitch, s32 volume) {
    const EffectCueBank *cue = &g_EffectCueTable[bank];
    EffectVoiceState state = VoicePairMatchesBank(voiceStart, bank)
                                 ? EFFECT_VOICE_UPDATE
                                 : EFFECT_VOICE_START;
    s32 voice;

    for (voice = 0; voice < CueVoiceCount(cue); voice++) {
        EffectVoice *effect = &g_EffectVoices[voiceStart + voice];

        effect->state = state;
        effect->note.value = cue->programs[voice].note;
        effect->tone = cue->programs[voice].tone;
        effect->pitch.value = pitch;
        effect->volume = volume * cue->volumeScale / 128;
    }
}

void SetPitchedSoundCue(s32 bank, s32 pitch, s32 volume) {
    s32 voiceStart;

    if (bank < 0) {
        bank = 0;
    } else if (bank >= EFFECT_CUE_BANK_COUNT) {
        bank = EFFECT_CUE_BANK_COUNT - 1;
    }
    volume = ClampCueLevel(volume);
    voiceStart = CueBankVoiceStart(bank);

    if (volume > 0) {
        StartCueVoices(voiceStart, bank, pitch, volume);
    } else if (bank == 0 || VoicePairMatchesBank(voiceStart, bank)) {
        if (g_EffectVoices[voiceStart].note.value >= 0 ||
            g_EffectVoices[voiceStart + 1].note.value >= 0) {
            ResetCueVoices(voiceStart,
                           CueVoiceCount(&g_EffectCueTable[bank]));
        }
    }
}
