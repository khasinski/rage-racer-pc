#include "game/audio.h"
#include "game/menu.h"

void ApplyAudioSettings(void) {
    SetSequenceVolumeSetting(g_BgmVolumeSetting);
    SetEffectVolumeSetting(g_SfxVolumeSetting);
    if (g_MonoOutput == 0) {
        SetStereoOutput();
    } else {
        SetMonoOutput();
    }
}
