#include <stdio.h>
#include "game/audio.h"
#include "game/audio_state_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    FIRST_SOUND_SLOT_VOICE = 14,
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

    for (i = 0; i < 5; i++) {
        SetSoundSlotVoiceEnabled(i, enabled);
    }
}

void SetEffectVoicesEnabled(s32 enabled) {
    SetSoundSlotVoicesEnabled(enabled);
}

void ResetSoundState(void) {
    s32 i;

    for (i = 0; i < 6; i++) {
        g_EngineSoundState.slotActive[i] = 0;
    }

    for (i = 0; i < 2; i++) {
        g_MusicChannels[i].mode = -1;
        g_MusicChannels[i].left.value = -1;
        g_MusicChannels[i].right.value = -1;
        g_MusicChannels[i].volLeft = 0;
        g_MusicChannels[i].volRight = 0;
    }

    for (i = 0; i < 4; i++) {
        g_EffectVoices[i].state = -1;
        g_EffectVoices[i].note.value = -1;
        g_EffectVoices[i].tone = -1;
        g_EffectVoices[i].pitch.value = 0x1E00;
        g_EffectVoices[i].volume = 0;
    }

    g_EngineSoundState.bank = -1;
    g_PanVoiceVolumeR = -1;
    g_PanVoiceVolumeL = -1;
    g_IndexedEffectIndexPrev = -1;
    g_IndexedEffectIndex = -1;
    g_IndexedEffectPitch = 0x1E00;
    g_SoundScale.scale = 0x80;
    g_PanVoiceActive = 0;
    g_EngineSoundState.volumeScale = 0x80;
    g_AudioLoadedSlotMask = 1;
}

s32 InitSoundWithVab(u8 *header, u8 *body) {
    s16 *vabIdPtr = g_SoundScale.vabIds;
    s16 vabId;

    SsSetTableSize((char *)GetSndTableArea(), 2, 1);
    /* The native port owns the sequence clock in TickSequenceAudio; it does
     * not install a simulated PlayStation counter interrupt. */
    SsSetTickMode(SS_NOTICK);
    SsSetVoiceCount(0xA);
    SsUtReverbOff();
    SetReverbPreset(2, 0, 0);
    ResetSoundState();

    *vabIdPtr = SsVabOpenHeadSticky(header, -1, 0x1000);
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
    SsSetMVol(0x3FFF, 0x3FFF);
    SsSetReservedVoice(0);
    return 0;
}

s32 InitSoundRuntime(void) {
    SsSetTableSize((char *)GetSndTableArea(), 2, 1);
    SsSetTickMode(SS_NOTICK);
    SsSetVoiceCount(0xA);
    SsUtReverbOff();
    SetReverbPreset(2, 0, 0);
    ResetSoundState();
    SsSetMVol(0x3FFF, 0x3FFF);
    SsSetReservedVoice(0);
    InitSequenceAudio();
    return 0;
}
