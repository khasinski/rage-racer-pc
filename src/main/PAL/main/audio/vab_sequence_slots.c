#include <stdio.h>
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

s32 OpenVabSequenceSlot(s32 slot, u8 *header, u8 *body, void *seq) {
    if (slot != AUDIO_SLOT_SEQUENCE) {
        return -1;
    }

    g_AudioLoadSlot = slot;
    g_SoundScale.vabIds[slot] =
        SsVabOpenHeadSticky(header, -1, g_VabSpuAddress[slot]);
    s16 vabId = g_SoundScale.vabIds[slot];
    if (vabId == -1) {
        printf("%s", g_MsgSeqVabOpenHeadError);
        BiosExit(1);
    }

    vabId = SsVabTransBody(body, vabId);
    g_SoundScale.vabIds[slot] = vabId;
    if (vabId == -1) {
        printf("%s", g_MsgSeqVabTransBodyError);
        BiosExit(1);
    }

    g_SeqHandle.storage = (s16)SsSeqOpen(seq, vabId);
    g_SeqVolumeFadeStep = 0;
    g_VabTransferDone = SsVabTransCompleted(0);
    return g_VabTransferDone;
}

s32 CloseAudioSlot(s32 slot) {
    if (slot != AUDIO_SLOT_SEQUENCE) {
        return 0;
    }

    s32 bit = 1 << slot;

    if ((bit & g_AudioLoadedSlotMask) == 0) {
        return 0;
    }

    g_AudioLoadedSlotMask &= ~bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsSeqCloseWrapper(g_SeqHandle.value);
    SsVabClose(g_SoundScale.vabIds[slot]);
    return 1;
}
