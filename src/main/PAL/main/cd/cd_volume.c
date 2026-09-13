#include "game/cd.h"
#include "game/cd_internal.h"

enum {
    CD_VOLUME_SETTING_MAX = 15,
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

static u32 ScaleCdMixLevel(u8 presetLevel, s32 volume) {
    return (presetLevel * volume / CD_VOLUME_MAX) << CD_MIX_FRACTION_BITS;
}

void SetCdVolume(s32 volume) {
    volume = ClampCdVolume(volume);

    g_Cd.volume = volume;
    g_Cd.mix.ll = g_Cd.fullMix.ll =
        ScaleCdMixLevel(g_CdMixPresets[0], volume);
    g_Cd.mix.lr = g_Cd.fullMix.lr =
        ScaleCdMixLevel(g_CdMixPresets[1], volume);
    g_Cd.mix.rr = g_Cd.fullMix.rr =
        ScaleCdMixLevel(g_CdMixPresets[2], volume);
    g_Cd.mix.rl = g_Cd.fullMix.rl =
        ScaleCdMixLevel(g_CdMixPresets[3], volume);

    StepCdVolumeFade();
}

void SetCdVolumeSetting(s32 level) {
    level = ClampCdVolumeSetting(level);
    SetCdVolume(level * CD_VOLUME_MAX / CD_VOLUME_SETTING_MAX);
}
