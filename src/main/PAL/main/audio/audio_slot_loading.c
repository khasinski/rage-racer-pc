#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    MAIN_CUE_AUDIO_SLOT = 0,
    SEQUENCE_AUDIO_SLOT = 1,
    RACE_CUE_AUDIO_SLOT = 2,
    ENGINE_AUDIO_SLOT = 3,
    ALTERNATE_SEQUENCE_AUDIO_SLOT = 6,
};

static s16 TransferVabToSlot(s32 slot, u8 *header, u8 *body,
                             s32 spuAddress) {
    s16 vabId = SsVabOpenHeadSticky(header, -1, spuAddress);

    if (vabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        BiosExit(1);
    }

    vabId = SsVabTransBody(body, vabId);
    if (vabId == -1) {
        printf("%s", g_MsgVabTransBodyError);
        BiosExit(1);
    }

    g_SoundScale.vabIds[slot] = vabId;
    return vabId;
}

s32 StartAudioSlotLoad(s32 slot, u8 *header, u8 *body, u16 *table) {
    if (slot == ENGINE_AUDIO_SLOT) {
        return StartVabTransferWithTable(header, body, table);
    }
    if (slot == SEQUENCE_AUDIO_SLOT ||
        slot == ALTERNATE_SEQUENCE_AUDIO_SLOT) {
        return OpenVabSequenceSlot(slot, header, body, table);
    }

    g_AudioLoadSlot = slot;
    TransferVabToSlot(slot, header, body, g_VabSpuAddress[slot]);

    g_VabTransferDone = SsVabTransCompleted(0);
    return g_VabTransferDone;
}

s32 PollAudioSlotLoad(void) {
    s16 completed;
    s32 slot;

    completed = SsVabTransCompleted(0);
    g_VabTransferDone = completed;

    if (completed != 0) {
        slot = g_AudioLoadSlot;
        g_AudioLoadedSlotMask |= 1 << slot;

        if (slot == MAIN_CUE_AUDIO_SLOT) {
            g_SoundCueBank = 1;
        } else if (slot == SEQUENCE_AUDIO_SLOT) {
            g_SoundCueBank = slot;
        } else if (slot == RACE_CUE_AUDIO_SLOT ||
                   slot == ENGINE_AUDIO_SLOT) {
            g_SoundCueBank = 2;
        }
    }

    return completed;
}

s32 CloseVabOnlyAudioSlot(s32 slot) {
    s32 bit = 1 << slot;

    if ((bit & g_AudioLoadedSlotMask) == 0) {
        return 0;
    }

    g_AudioLoadedSlotMask &= ~bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsVabClose(g_SoundScale.vabIds[slot]);
    return 1;
}

s32 CloseLoadedAudioSlots(void) {
    SpuVmDamperStep();
    if (CloseAudioSlot(SEQUENCE_AUDIO_SLOT) == 0) {
        return 0;
    }
    if (CloseVabOnlyAudioSlot(RACE_CUE_AUDIO_SLOT) == 0) {
        return 0;
    }
    if (CloseVabOnlyAudioSlot(ENGINE_AUDIO_SLOT) == 0) {
        return 0;
    }
    return 1;
}

s32 StartVabTransferWithTable(u8 *header, u8 *body, u16 *table) {
    g_AudioLoadSlot = ENGINE_AUDIO_SLOT;
    TransferVabToSlot(ENGINE_AUDIO_SLOT, header, body,
                      g_VabSpuAddress[ENGINE_AUDIO_SLOT]);

    if (table != NULL) {
        LoadAudioParameterTable(table);
    }

    g_EngineSoundState.extraVabLoaded = 1;
    g_VabTransferDone = SsVabTransCompleted(0);
    return g_VabTransferDone;
}
