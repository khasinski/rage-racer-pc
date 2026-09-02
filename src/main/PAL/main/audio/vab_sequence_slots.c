#include <stdio.h>
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

s32 OpenSequenceAudioSlot(u8 *header, u8 *body, void *seq) {
    s16 vabId;

    g_AudioLoadSlot = AUDIO_SLOT_SEQUENCE;
    vabId = SsVabOpenHeadSticky(
        header, -1, g_VabSpuAddress[AUDIO_SLOT_SEQUENCE]);
    if (vabId == -1) {
        printf("%s", g_MsgSeqVabOpenHeadError);
        BiosExit(1);
    }

    vabId = SsVabTransBody(body, vabId);
    if (vabId == -1) {
        printf("%s", g_MsgSeqVabTransBodyError);
        BiosExit(1);
    }
    g_SoundScale.vabIds[AUDIO_SLOT_SEQUENCE] = vabId;

    g_SeqHandle.storage = SsSeqOpen(seq, vabId);
    g_SeqVolumeFadeStep = 0;
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
