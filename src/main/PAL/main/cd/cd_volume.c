#include "game/cd.h"
#include "game/cd_internal.h"

static u32 ScaleCdMixLevel(u8 presetLevel, s32 volume) {
    return (presetLevel * volume / 127) << 12;
}

void SetCdVolume(s32 volume) {
    const s32 presetOffset = g_CdMixPreset * 4;

    g_CdVolume = volume;
    g_CdMixLL = g_CdMixFullLL =
        ScaleCdMixLevel(g_CdMixPresets[presetOffset], volume);
    g_CdMixLR = g_CdMixFullLR =
        ScaleCdMixLevel(g_CdMixPresets[presetOffset + 1], volume);
    g_CdMixRR = g_CdMixFullRR =
        ScaleCdMixLevel(g_CdMixPresets[presetOffset + 2], volume);
    g_CdMixRL = g_CdMixFullRL =
        ScaleCdMixLevel(g_CdMixPresets[presetOffset + 3], volume);

    StepCdVolumeFade();
}

void SetCdVolumeSetting(s32 level) {
    SetCdVolume(level * 127 / 15);
}
