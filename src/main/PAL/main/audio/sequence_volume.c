#include "game/audio.h"
#include "game/sound.h"

void RefreshSequenceVolumeScale(void) {
    s32 temp = g_SeqVolumeSetting * 114;

    SetSequenceVolume(temp / 15);
}

void SetSequenceVolumeScale(s32 scale) {
    s32 temp = scale * 114;

    g_SeqVolumeSetting = scale;
    SetSequenceVolume(temp / 15);
}
