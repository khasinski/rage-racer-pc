#include "common.h"
#include <stdio.h>
#include "game/audio.h"
#include "game/audio_state_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"
#include "psyq/kernel.h"
#include "game/cd.h"

void SetSoundSlotVoiceEnabled(s32 slot, s32 enabled) {
    s32 *entry;

    if (enabled != 0) {
        s32 *base = g_EngineSoundState.slotActive;
        entry = base + slot;
        if (*entry == 0) {
            PlaySoundSlotVoice(slot, 0, 3);
            *entry = 1;
        }
    } else {
        s32 *base = g_EngineSoundState.slotActive;
        entry = base + slot;
        if (*entry != 0) {
            StopSoundSlotVoice(slot);
            *entry = 0;
        }
    }
}

void SetSoundSlotVoicesEnabled(s32 enabled) {
    s32 i;

    for (i = 0; i < 6; i++) {
        if (i != 5) {
            SetSoundSlotVoiceEnabled(i, enabled);
        }
    }
}

void SetEffectVoicesEnabled(s32 enabled) {
    SetSoundSlotVoicesEnabled(enabled);
}

void ResetSoundState(void) {
    s32 i;

    {
        s32 *ptr;

        i = 5;
        ptr = &g_EngineSoundState.slotActive[5];
        for (; i >= 0; i--) {
            *ptr-- = 0;
        }
    }

    {
        s32 neg;

        i = 0;
        neg = -1;
        for (; i < 2; i++) {
            g_MusicChannels[i].mode = neg;
            g_MusicChannels[i].left.value = neg;
            g_MusicChannels[i].right.value = neg;
            g_MusicChannels[i].volLeft.value = 0;
            /* These are fields of the PS1's contiguous audio work area. Do
             * not reconstruct their addresses from a separately linked host
             * global: that can overwrite unrelated game state. */
            g_MusicChannels[i].volRight.value = 0;
        }
    }

    {
        s32 offset;

        {
            s32 i;
            s32 neg;
            s32 value;

            i = 0;
            neg = -1;
            value = 0x1E00;
            offset = 0;
            for (; i < 4; i++) {
                GetEffectVoiceAtByteOffset(offset)->state = neg;
                GetEffectVoiceAtByteOffset(offset)->note.value = neg;
                GetEffectVoiceAtByteOffset(offset)->tone = neg;
                GetEffectVoiceAtByteOffset(offset)->pitch.value = value;
                GetEffectVoiceAtByteOffset(offset)->volume = 0;
                offset += sizeof(EffectVoice);
            }
        }

        {
            s32 value;

            offset = 0x80;
            value = -1;
            g_EngineSoundState.bank = value;
            g_PanVoiceVolumeR = value;
            g_PanVoiceVolumeL = value;
            g_IndexedEffectIndexPrev = value;
            g_IndexedEffectIndex = value;
            value = 0x1E00;
            g_IndexedEffectPitch = value;
            value = 1;
            g_SoundScale.scale = offset;
            g_PanVoiceActive = 0;
            g_EngineSoundState.volumeScale = offset;
            g_AudioLoadedSlotMask = value;
        }
    }
}

s32 InitSoundWithVab(u8 *header, u8 *body) {
    s16 *vabIdPtr = g_SoundScale.vabIds;
    s16 vabId;

    SsSetTableSize((u8 *)GetSndTableArea(), 2, 1);
    SsSetTickMode(1);
    SsStartSoundTickMode1();
    SsSetVoiceCount(0xA);
    SsUtReverbOff();
    SetReverbPreset(2, 0, 0);
    ResetSoundState();

    *vabIdPtr = SsVabOpenHeadSticky(header, -1, 0x1000);
    vabId = *vabIdPtr;
    if (vabId == -1) {
        printf(g_MsgVabOpenHeadError);
        BiosExit(1);
    }

    *vabIdPtr = SsVabTransBody(body, vabId);
    if (*vabIdPtr == -1) {
        printf(g_MsgVabTransBodyError);
        BiosExit(1);
    }

    SsVabTransCompleted(1);
    SsSetMVol(0x3FFF, 0x3FFF);
    SsSetReservedVoice(0);
    return 0;
}

s32 InitSoundRuntime(void) {
    SsSetTableSize((u8 *)GetSndTableArea(), 2, 1);
    SsSetTickMode(0x1000);
    SsStartSoundTickMode1();
    SsSetVoiceCount(0xA);
    SsUtReverbOff();
    SetReverbPreset(2, 0, 0);
    ResetSoundState();
    SsSetMVol(0x3FFF, 0x3FFF);
    SsSetReservedVoice(0);
    InitSequenceAudio();
    return 0;
}
