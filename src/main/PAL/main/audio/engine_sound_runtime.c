#include "game/audio.h"
#include "game/sound.h"

void UpdateLoadedAudioVoices(s32 rpm, s32 bank) {
    s32 position = rpm * 10240 / g_EngineSoundState.maxRpm;
    s32 slot;

    if (bank != g_EngineSoundState.bank) {
        for (slot = 0; slot < 6; slot++) {
            if (g_EngineSoundState.slotActive[slot] != 0 &&
                g_SoundSlotTone[slot][0] != g_SoundSlotTone[slot][1]) {
                PlaySoundSlotVoice(slot, bank, 3);
            }
        }
        g_EngineSoundState.bank = bank;
    }

    for (slot = 0; slot < 6; slot++) {
        if (g_EngineSoundState.slotActive[slot] != 0) {
            s32 bend = InterpolateAudioParameter(slot * 2, position, bank);
            s32 volume = InterpolateAudioParameter(slot * 2 + 1,
                                                   position, bank);

            volume = volume * g_EngineSoundState.volumeScale / 128;
            SetSoundSlotTone(slot, bend, volume, bank, 3);
        }
    }

    g_EngineSoundState.position = position;
    ApplyPanVoiceVolume();
    UpdateBasicEffectVoices();
    UpdateIndexedEffectVoice();
    UpdateEffectVoiceStates();
}
