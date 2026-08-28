#include <libsnd.h>
#include "game/audio.h"
#include "game/sound.h"

typedef union SequenceVolumeAddress {
    s32 *value;
    s16 *sdkVolume;
} SequenceVolumeAddress;

void ApplyCurrentSequenceAudio(void) {
    SequenceVolumeAddress volumeAddress;

    volumeAddress.value = &g_SeqVolume;
    SsSeqSetVol(g_SeqHandle.value, *volumeAddress.sdkVolume, *volumeAddress.sdkVolume);
    SetReverbDepth(0x28, 0x28);
}

void SetMasterVolumeMono(s16 volume) { SsSetMVol(volume, volume); }
