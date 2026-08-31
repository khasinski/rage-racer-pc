#include <stdio.h>
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"


s32 OpenVabSequenceSlot(s32 slot, u8 *header, u8 *body, void *seq) {
    s16 vabId;
    s32 vabIdAgain;

    g_AudioLoadSlot = slot;
    g_SoundScale.vabIds[slot] = SsVabOpenHeadSticky(header, -1, g_VabSpuAddress[slot]);
    vabId = g_SoundScale.vabIds[slot];
    if (vabId == -1) {
        printf("%s", g_MsgSeqVabOpenHeadError);
        BiosExit(1);
    }

    g_SoundScale.vabIds[slot] = SsVabTransBody(body, vabId);
    vabIdAgain = g_SoundScale.vabIds[slot];
    if (vabIdAgain == -1) {
        printf("%s", g_MsgSeqVabTransBodyError);
        BiosExit(1);
    }

    g_SeqHandle.storage = (s16)SsSeqOpen(seq, vabIdAgain);
    g_SeqVolumeFadeStep = 0;
    g_VabTransferDone = SsVabTransCompleted(0);
    return g_VabTransferDone;
}

s32 CloseAudioSlot(s32 slot) {
    s32 bit = 1 << slot;

    if ((bit & g_AudioLoadedSlotMask) == 0) {
        return 0;
    }

    g_AudioLoadedSlotMask ^= bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsSeqCloseWrapper(g_SeqHandle.value);
    SsVabClose(g_SoundScale.vabIds[slot]);
    return 1;
}

void StartVabSlotVoice(s32 voice, s32 unused, s16 vabSlot) {
    (void)unused;
    VabSlotVoice *slotVoice = &g_VabSlotVoices[voice];

    SsUtKeyOnV((s16)voice, g_SoundScale.vabIds[(s16)vabSlot], slotVoice->tone, 0, 0x3C, 0,
               0, 0);
}

void StopDirectVoice(s32 voice) {
    SsUtKeyOffV((s16)voice);
}
