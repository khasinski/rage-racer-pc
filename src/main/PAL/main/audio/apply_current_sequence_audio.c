#include <libsnd.h>
#include "game/audio.h"
#include "game/sound.h"

void ApplyCurrentSequenceAudio(void) {
    s16 volume = (s16)g_SeqVolume;

    SsSeqSetVol(g_SeqHandle.value, volume, volume);
    SetReverbDepth(0x28, 0x28);
}
