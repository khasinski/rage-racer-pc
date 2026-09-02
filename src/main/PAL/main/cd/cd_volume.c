#include "game/cd.h"
#include "game/cd_internal.h"

enum {
    CD_VOLUME_SETTING_MAX = 15,
    CD_MIX_PRESET_COUNT = 2,
    CD_MIX_CHANNEL_COUNT = 4,
};

static s32 ClampCdVolume(s32 volume) {
    if (volume < 0) {
        return 0;
    }
    return volume > CD_VOLUME_MAX ? CD_VOLUME_MAX : volume;
}

static s32 ClampCdVolumeSetting(s32 setting) {
    if (setting < 0) {
        return 0;
    }
    return setting > CD_VOLUME_SETTING_MAX ? CD_VOLUME_SETTING_MAX : setting;
}

static s32 ClampCdMixPreset(s32 preset) {
    if (preset < 0) {
        return 0;
    }
    return preset >= CD_MIX_PRESET_COUNT ? CD_MIX_PRESET_COUNT - 1 : preset;
}

static u32 ScaleCdMixLevel(u8 presetLevel, s32 volume) {
    return (presetLevel * volume / CD_VOLUME_MAX) << CD_MIX_FRACTION_BITS;
}

void SetCdVolume(s32 volume) {
    s32 presetOffset;

    volume = ClampCdVolume(volume);
    g_CdMixPreset = ClampCdMixPreset(g_CdMixPreset);
    presetOffset = g_CdMixPreset * CD_MIX_CHANNEL_COUNT;

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
    level = ClampCdVolumeSetting(level);
    SetCdVolume(level * CD_VOLUME_MAX / CD_VOLUME_SETTING_MAX);
}

void SetCdMixPreset(s32 preset) {
    g_CdMixPreset = ClampCdMixPreset(preset);
    SetCdVolume(g_CdVolume);
}
