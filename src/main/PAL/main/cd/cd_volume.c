#include "game/cd.h"
#include "game/cd_internal.h"


void SetCdVolume(s32 volume) {
    s32 offset;
    s32 scale;
    s32 product;
    s32 value;

    offset = g_CdMixPreset;
    g_CdVolume = volume;
    scale = g_CdVolume;
    offset <<= 2;

    product = g_CdMixPresets[offset] * scale;
    value = (product / 127) << 12;
    g_CdMixFullLL = value;
    g_CdMixLL = value;

    product = g_CdMixPresets[offset + 1] * scale;
    value = (product / 127) << 12;
    g_CdMixFullLR = value;
    g_CdMixLR = value;

    product = g_CdMixPresets[offset + 2] * scale;
    value = (product / 127) << 12;
    g_CdMixFullRR = value;
    g_CdMixRR = value;

    product = g_CdMixPresets[offset + 3] * scale;
    value = (product / 127) << 12;
    g_CdMixFullRL = value;
    g_CdMixRL = value;

    StepCdVolumeFade();
}

void SetCdVolumeSetting(s32 level) {
    s32 product = (level << 7) - level;

    g_CdVolume = product / 15;
    SetCdVolume(g_CdVolume);
}
