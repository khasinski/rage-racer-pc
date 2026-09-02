#include "game/audio.h"
#include "game/sound.h"

enum {
    ENGINE_SOUND_VAB_SLOT = 3,
    PARAMETERS_PER_ENGINE_SLOT = 2,
    ENGINE_SLOT_BEND_PARAMETER = 0,
    ENGINE_SLOT_VOLUME_PARAMETER = 1,
};

void ForceSoundSlotVoicePlayback(s32 enabled) {
    s32 slot;

    SetSoundSlotVoicesEnabled(enabled);

    if (enabled != 0) {
        for (slot = 0; slot < ENGINE_SOUND_SLOT_COUNT; slot++) {
            if (g_EngineSoundState.slotActive[slot] != 0 &&
                g_SoundSlotTone[slot][0] != g_SoundSlotTone[slot][1]) {
                PlaySoundSlotVoice(slot, g_EngineSoundState.bank,
                                   ENGINE_SOUND_VAB_SLOT);
            }
        }

        for (slot = 0; slot < ENGINE_SOUND_SLOT_COUNT; slot++) {
            if (g_EngineSoundState.slotActive[slot] != 0) {
                s32 bend = InterpolateAudioParameter(
                    slot * PARAMETERS_PER_ENGINE_SLOT +
                        ENGINE_SLOT_BEND_PARAMETER,
                    g_EngineSoundState.position,
                    g_EngineSoundState.bank);
                s32 volume = InterpolateAudioParameter(
                    slot * PARAMETERS_PER_ENGINE_SLOT +
                        ENGINE_SLOT_VOLUME_PARAMETER,
                    g_EngineSoundState.position,
                    g_EngineSoundState.bank);

                volume = volume * g_EngineSoundState.volumeScale / 128;
                SetSoundSlotTone(slot, bend, volume,
                                 g_EngineSoundState.bank,
                                 ENGINE_SOUND_VAB_SLOT);
            }
        }
    }
}

void ForceAllEffectVoicesEnabled(s32 enabled) {
    ForcePanVoiceEnabled(enabled);
    ForceBasicEffectVoicesEnabled(enabled);
    ForceIndexedEffectVoiceEnabled(enabled);
    ForcePitchEffectVoicesEnabled(enabled);
    ForceSoundSlotVoicePlayback(enabled);
}
