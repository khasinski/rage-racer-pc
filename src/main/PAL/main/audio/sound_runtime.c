#include "game/audio.h"
#include "game/audio_state_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    FIRST_SOUND_SLOT_VOICE = 14,
    AUTO_SOUND_SLOT_COUNT = 5,
    SOUND_SLOT_COUNT = 6,
    SOUND_RUNTIME_VOICE_COUNT = 10,
    DEFAULT_SOUND_SCALE = 128,
};

static void StopSoundSlotVoice(s32 slot) {
    SsUtKeyOffV((s16)(slot + FIRST_SOUND_SLOT_VOICE));
}

static void SetSoundSlotVoiceEnabled(s32 slot, s32 enabled) {
    s32 *entry = &g_EngineSoundState.slotActive[slot];

    if (enabled != 0) {
        if (*entry == 0) {
            PlaySoundSlotVoice(slot, 0, 3);
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

    for (i = 0; i < SOUND_SLOT_COUNT; i++) {
        g_EngineSoundState.slotActive[i] = 0;
    }

    ResetAudioVoiceState();

    g_EngineSoundState.bank = -1;
    g_SoundScale.scale = DEFAULT_SOUND_SCALE;
    g_EngineSoundState.volumeScale = DEFAULT_SOUND_SCALE;
    g_AudioLoadedSlotMask = 1;
}

static void PrepareSoundRuntime(void) {
    SsSetTableSize((char *)GetSndTableArea(), 2, 1);
    /* The native port owns the sequence clock in TickSequenceAudio; it does
     * not install a simulated PlayStation counter interrupt. */
    SsSetTickMode(SS_NOTICK);
    SsSetVoiceCount(SOUND_RUNTIME_VOICE_COUNT);
    SsUtReverbOff();
    SetReverbPreset(2, 0, 0);
    ResetSoundState();
}

static void FinishSoundRuntimeInitialization(void) {
    SsSetMVol(0x3FFF, 0x3FFF);
    SsSetReservedVoice(0);
}

void InitSoundRuntime(void) {
    PrepareSoundRuntime();
    FinishSoundRuntimeInitialization();
    InitSequenceAudio();
}
