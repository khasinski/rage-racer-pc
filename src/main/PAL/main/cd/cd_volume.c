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

void SetCdVolume(s32 volume) {
    u32 level;

    volume = ClampCdVolume(volume);
    level = (u32)volume << CD_MIX_FRACTION_BITS;

    g_Cd.volume = (u8)volume;
    g_Cd.mix.ll = g_Cd.fullMix.ll = level;
    g_Cd.mix.lr = g_Cd.fullMix.lr = 0;
    g_Cd.mix.rr = g_Cd.fullMix.rr = level;
    g_Cd.mix.rl = g_Cd.fullMix.rl = 0;

    StepCdVolumeFade();
}

void SetCdVolumeSetting(s32 level) {
    level = ClampCdVolumeSetting(level);
    SetCdVolume(level * CD_VOLUME_MAX / CD_VOLUME_SETTING_MAX);
}
