#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/audio_state_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    FIRST_SOUND_SLOT_VOICE = 14,
    AUTO_SOUND_SLOT_COUNT = ENGINE_SOUND_SLOT_COUNT - 1,
    SOUND_RUNTIME_VOICE_COUNT = 10,
    DEFAULT_SOUND_SCALE = 128,
    SOUND_TABLE_SEQUENCE_COUNT = 2,
    SOUND_TABLE_TRACK_COUNT = 1,
    SOUND_RUNTIME_REVERB_PRESET = 2,
    SOUND_MASTER_VOLUME = 0x3FFF,
};

static void StopSoundSlotVoice(s32 slot) {
    SsUtKeyOffV((s16)(slot + FIRST_SOUND_SLOT_VOICE));
}

static void SetSoundSlotVoiceEnabled(s32 slot, s32 enabled) {
    s32 *entry = &g_EngineSoundState.slotActive[slot];

    if (enabled != 0) {
        if (*entry == 0) {
            PlaySoundSlotVoice(slot, 0, AUDIO_SLOT_ENGINE);
            *entry = 1;
        }
    } else {
        if (*entry != 0) {
            StopSoundSlotVoice(slot);
            *entry = 0;
        }
    }
}

void SetSoundSlotVoicesEnabled(s32 enabled) {
    s32 i;

    /* Slot 5 is managed by ForceSoundSlotVoicePlayback and intentionally
     * excluded here, as in the retail i != 5 loop. */
    for (i = 0; i < AUTO_SOUND_SLOT_COUNT; i++) {
        SetSoundSlotVoiceEnabled(i, enabled);
    }
}

static void ResetSoundState(void) {
    s32 i;

    for (i = 0; i < ENGINE_SOUND_SLOT_COUNT; i++) {
        g_EngineSoundState.slotActive[i] = 0;
    }

    ResetAudioVoiceState();

    g_EngineSoundState.bank = -1;
    g_SoundScale.scale = DEFAULT_SOUND_SCALE;
    g_EngineSoundState.volumeScale = DEFAULT_SOUND_SCALE;
    g_AudioLoadedSlotMask = 1;
    g_AudioLoadSlot = -1;
}

void InitSoundRuntime(void) {
    SsSetTableSize((char *)GetSndTableArea(), SOUND_TABLE_SEQUENCE_COUNT,
                   SOUND_TABLE_TRACK_COUNT);
    /* The native port owns the sequence clock in TickSequenceAudio; it does
     * not install a simulated PlayStation counter interrupt. */
    SsSetTickMode(SS_NOTICK);
    SsSetReservedVoice(SOUND_RUNTIME_VOICE_COUNT);
    SsUtReverbOff();
    SetReverbPreset(SOUND_RUNTIME_REVERB_PRESET, 0, 0);
    ResetSoundState();
    SsSetMVol(SOUND_MASTER_VOLUME, SOUND_MASTER_VOLUME);
    SsSetReservedVoice(0);
    InitSequenceAudio();
}
