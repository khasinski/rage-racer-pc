#include <stdio.h>
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

s32 OpenSequenceAudioSlot(u8 *header, u8 *body, void *seq) {
    s16 openedVabId;
    s16 vabId;
    s16 sequenceHandle;

    openedVabId = SsVabOpenHeadSticky(
        header, -1, g_VabSpuAddress[AUDIO_SLOT_SEQUENCE]);
    if (openedVabId == -1) {
        printf("%s", g_MsgSeqVabOpenHeadError);
        return -1;
    }

    vabId = SsVabTransBody(body, openedVabId);
    if (vabId == -1) {
        SsVabClose(openedVabId);
        printf("%s", g_MsgSeqVabTransBodyError);
        return -1;
    }
    g_SoundScale.vabIds[AUDIO_SLOT_SEQUENCE] = vabId;

    sequenceHandle = SsSeqOpen(seq, vabId);
    if (sequenceHandle == -1) {
        SsVabClose(vabId);
        return -1;
    }
    g_SeqHandle.storage = sequenceHandle;
    g_SeqVolumeFadeStep = 0;
    g_AudioLoadSlot = AUDIO_SLOT_SEQUENCE;
    return SsVabTransCompleted(0);
}

s32 CloseSequenceAudioSlot(void) {
    s32 bit = 1 << AUDIO_SLOT_SEQUENCE;

    if ((bit & g_AudioLoadedSlotMask) == 0) {
        return 0;
    }

    g_AudioLoadedSlotMask &= ~bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsSeqCloseWrapper(g_SeqHandle.value);
    SsVabClose(g_SoundScale.vabIds[AUDIO_SLOT_SEQUENCE]);
    return 1;
}
