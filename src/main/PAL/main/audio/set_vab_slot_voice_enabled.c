#include "game/audio.h"
#include "game/sound.h"

void SetSequenceVolume(s32 volume) {
    g_SeqVolume = volume;
    SsSeqSetVol(g_SeqHandle.value, (s16)volume, (s16)volume);
}
