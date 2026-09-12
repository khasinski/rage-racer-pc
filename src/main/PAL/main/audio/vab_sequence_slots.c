#include <stdio.h>
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

s32 OpenSequenceAudioSlot(u8 *header, u8 *body, void *seq) {
    s16 openedVabId;
    s16 vabId;
    s16 sequenceHandle;

    openedVabId = SsVabOpenHeadSticky(
        header, -1, g_VabSpuAddress[AUDIO_SLOT_SEQUENCE]);
    if (openedVabId == -1) {
        printf("SsVabOpenHead Error\n");
        return -1;
    }

    vabId = SsVabTransBody(body, openedVabId);
    if (vabId == -1) {
        SsVabClose(openedVabId);
        printf("SsVabTransBody Error\n");
        return -1;
    }
    sequenceHandle = SsSeqOpen(seq, vabId);
    if (sequenceHandle == -1) {
        SsVabClose(vabId);
        return -1;
    }
    g_SoundScale.vabIds[AUDIO_SLOT_SEQUENCE] = vabId;
    g_Audio.seq.handle = sequenceHandle;
    g_Audio.seq.fade = 0;
    g_Audio.slots.loading = AUDIO_SLOT_SEQUENCE;
    return SsVabTransCompleted(0);
}

void CloseSequenceAudioSlot(void) {
    s32 bit = 1 << AUDIO_SLOT_SEQUENCE;

    if ((bit & g_Audio.slots.loaded) == 0) {
        return;
    }

    g_Audio.slots.loaded &= ~bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsSeqClose((s16)g_Audio.seq.handle);
    SsVabClose(g_SoundScale.vabIds[AUDIO_SLOT_SEQUENCE]);
}
