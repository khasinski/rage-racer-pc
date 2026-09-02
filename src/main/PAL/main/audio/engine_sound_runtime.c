#include "game/audio.h"
#include "game/sound.h"

enum {
    ENGINE_SOUND_POSITION_RANGE = 10240,
    ENGINE_SOUND_VOLUME_ONE = 128,
    PARAMETERS_PER_SOUND_SLOT = 2,
};

static s32 EngineSoundPositionForRpm(s32 rpm) {
    return rpm * ENGINE_SOUND_POSITION_RANGE / g_EngineSoundState.maxRpm;
}

static s32 SlotParameterIndex(s32 slot, s32 parameterOffset) {
    return slot * PARAMETERS_PER_SOUND_SLOT + parameterOffset;
}

void UpdateLoadedAudioVoices(s32 rpm, s32 bank) {
    s32 position = EngineSoundPositionForRpm(rpm);
    s32 slot;

    if (bank != g_EngineSoundState.bank) {
        for (slot = 0; slot < ENGINE_SOUND_SLOT_COUNT; slot++) {
            if (g_EngineSoundState.slotActive[slot] != 0 &&
                g_SoundSlotTone[slot][0] != g_SoundSlotTone[slot][1]) {
                PlaySoundSlotVoice(slot, bank, AUDIO_SLOT_ENGINE);
            }
        }
        g_EngineSoundState.bank = bank;
    }

    for (slot = 0; slot < ENGINE_SOUND_SLOT_COUNT; slot++) {
        if (g_EngineSoundState.slotActive[slot] != 0) {
            s32 bend = InterpolateAudioParameter(
                SlotParameterIndex(slot, 0), position, bank);
            s32 volume = InterpolateAudioParameter(
                SlotParameterIndex(slot, 1), position, bank);

            volume = volume * g_EngineSoundState.volumeScale /
                     ENGINE_SOUND_VOLUME_ONE;
            SetSoundSlotTone(slot, bend, volume, bank,
                             AUDIO_SLOT_ENGINE);
        }
    }

    g_EngineSoundState.position = position;
    ApplyPanVoiceVolume();
    UpdateBasicEffectVoices();
    UpdateIndexedEffectVoice();
    UpdateEffectVoiceStates();
}
