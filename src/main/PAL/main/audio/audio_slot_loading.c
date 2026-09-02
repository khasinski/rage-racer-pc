#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

static s32 TransferVabToSlot(s32 slot, u8 *header, u8 *body,
                             s32 spuAddress) {
    s16 vabId = SsVabOpenHeadSticky(header, -1, spuAddress);

    if (vabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        return 0;
    }

    vabId = SsVabTransBody(body, vabId);
    if (vabId == -1) {
        printf("%s", g_MsgVabTransBodyError);
        return 0;
    }

    g_SoundScale.vabIds[slot] = vabId;
    return 1;
}

static s32 StartEngineAudioSlotLoad(u8 *header, u8 *body, u16 *table);

s32 StartAudioSlotLoad(s32 slot, u8 *header, u8 *body, u16 *table) {
    if (slot < 0 || slot >= AUDIO_SLOT_COUNT || header == NULL ||
        body == NULL) {
        return -1;
    }
    if (slot == AUDIO_SLOT_ENGINE) {
        return StartEngineAudioSlotLoad(header, body, table);
    }
    if (slot == AUDIO_SLOT_SEQUENCE) {
        if (table == NULL) return -1;
        return OpenSequenceAudioSlot(header, body, table);
    }

    g_AudioLoadSlot = slot;
    if (!TransferVabToSlot(slot, header, body, g_VabSpuAddress[slot])) {
        return -1;
    }

    return SsVabTransCompleted(0);
}

s32 PollAudioSlotLoad(void) {
    s32 completed;
    s32 slot;

    completed = SsVabTransCompleted(0);
    if (completed != 0) {
        slot = g_AudioLoadSlot;
        if ((u32)slot >= AUDIO_SLOT_COUNT) {
            return completed;
        }
        g_AudioLoadedSlotMask |= 1 << slot;

        if (slot == AUDIO_SLOT_MAIN_CUES) {
            g_SoundCueBank = 1;
        } else if (slot == AUDIO_SLOT_SEQUENCE) {
            g_SoundCueBank = slot;
        } else if (slot == AUDIO_SLOT_RACE_CUES ||
                   slot == AUDIO_SLOT_ENGINE) {
            g_SoundCueBank = 2;
        }
    }

    return completed;
}

static void CloseVabOnlyAudioSlot(s32 slot) {
    s32 bit;

    if (slot < 0 || slot >= AUDIO_SLOT_COUNT) {
        return;
    }

    bit = 1 << slot;

    if ((bit & g_AudioLoadedSlotMask) == 0) {
        return;
    }

    g_AudioLoadedSlotMask &= ~bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsVabClose(g_SoundScale.vabIds[slot]);
}

void CloseLoadedAudioSlots(void) {
    SpuVmDamperStep();
    CloseSequenceAudioSlot();
    CloseVabOnlyAudioSlot(AUDIO_SLOT_RACE_CUES);
    CloseVabOnlyAudioSlot(AUDIO_SLOT_ENGINE);
}

static s32 StartEngineAudioSlotLoad(u8 *header, u8 *body, u16 *table) {
    g_AudioLoadSlot = AUDIO_SLOT_ENGINE;
    if (!TransferVabToSlot(AUDIO_SLOT_ENGINE, header, body,
                           g_VabSpuAddress[AUDIO_SLOT_ENGINE])) {
        return -1;
    }

    if (table != NULL) {
        LoadAudioParameterTable(table);
    }

    g_EngineSoundState.extraVabLoaded = 1;
    return SsVabTransCompleted(0);
}
