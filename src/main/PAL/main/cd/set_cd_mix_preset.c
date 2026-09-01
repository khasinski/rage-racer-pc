#include "game/cd.h"
void SetCdMixPreset(s32 preset) {
    g_CdMixPreset = preset;
    SetCdVolume(g_CdVolume);
}
