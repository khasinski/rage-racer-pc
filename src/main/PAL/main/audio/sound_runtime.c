#include <stdio.h>
#include "game/audio.h"
#include "game/audio_state_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    FIRST_SOUND_SLOT_VOICE = 14,
    AUTO_SOUND_SLOT_COUNT = 5,
    SOUND_SLOT_COUNT = 6,
    MUSIC_CHANNEL_COUNT = 2,
    EFFECT_VOICE_COUNT = 4,
    SOUND_RUNTIME_VOICE_COUNT = 10,
    DEFAULT_SOUND_SCALE = 128,
    DEFAULT_EFFECT_PITCH = 0x1E00,
    MAIN_VAB_SPU_ADDRESS = 0x1000,
};

void StopSoundSlotVoice(s32 slot) {
    SsUtKeyOffV((s16)(slot + FIRST_SOUND_SLOT_VOICE));
}

void SetSoundSlotVoiceEnabled(s32 slot, s32 enabled) {
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

void SetEffectVoicesEnabled(s32 enabled) {
    SetSoundSlotVoicesEnabled(enabled);
}

void ResetSoundState(void) {
    s32 i;

    for (i = 0; i < SOUND_SLOT_COUNT; i++) {
        g_EngineSoundState.slotActive[i] = 0;
    }

    for (i = 0; i < MUSIC_CHANNEL_COUNT; i++) {
        g_MusicChannels[i].mode = -1;
        g_MusicChannels[i].left.value = -1;
        g_MusicChannels[i].right.value = -1;
        g_MusicChannels[i].volLeft = 0;
        g_MusicChannels[i].volRight = 0;
    }

    for (i = 0; i < EFFECT_VOICE_COUNT; i++) {
        g_EffectVoices[i].state = -1;
        g_EffectVoices[i].note.value = -1;
        g_EffectVoices[i].tone = -1;
        g_EffectVoices[i].pitch.value = DEFAULT_EFFECT_PITCH;
        g_EffectVoices[i].volume = 0;
    }

    g_EngineSoundState.bank = -1;
    g_PanVoiceVolumeR = -1;
    g_PanVoiceVolumeL = -1;
    g_IndexedEffectIndexPrev = -1;
    g_IndexedEffectIndex = -1;
    g_IndexedEffectPitch = DEFAULT_EFFECT_PITCH;
    g_SoundScale.scale = DEFAULT_SOUND_SCALE;
    g_PanVoiceActive = 0;
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

s32 InitSoundWithVab(u8 *header, u8 *body) {
    s16 *vabIdPtr = g_SoundScale.vabIds;
    s16 vabId;

    PrepareSoundRuntime();

    *vabIdPtr = SsVabOpenHeadSticky(header, -1, MAIN_VAB_SPU_ADDRESS);
    vabId = *vabIdPtr;
    if (vabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        BiosExit(1);
    }

    *vabIdPtr = SsVabTransBody(body, vabId);
    if (*vabIdPtr == -1) {
        printf("%s", g_MsgVabTransBodyError);
        BiosExit(1);
    }

    SsVabTransCompleted(1);
    FinishSoundRuntimeInitialization();
    return 0;
}

s32 InitSoundRuntime(void) {
    PrepareSoundRuntime();
    FinishSoundRuntimeInitialization();
    InitSequenceAudio();
    return 0;
}
