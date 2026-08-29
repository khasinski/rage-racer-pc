#include "game/audio.h"
#include "game/cd.h"
#include "game/sound.h"
#include "psyq/snd.h"

void SetLoadedTableVolumeScale(s32 scale) {
    scale = ClampVoiceVolume(scale);
    g_EngineSoundState.volumeScale = scale;
}

void SetSequenceVolumeSetting(s32 setting) {
    u32 adjusted;
    s32 value;

    value = setting;
    if (value >= 0) {
        adjusted = setting;
        adjusted++;
        adjusted--;
        setting = adjusted;
        if (setting >= 0x10) {
            setting = 0xF;
        }
    } else {
        setting = 0;
    }

    value = setting;
    SetCdVolumeSetting(setting);
    SetSequenceVolumeScale(value);
}

/* Set the effect master volume scale (g_SoundScale.scale) from a
 * 0..15 level, mapping it onto the 0..0x80 fixed-point scale used by the
 * effect-voice volume math. */
void SetEffectVolumeSetting(s32 level) {
    if (level >= 0) {
        if (level >= 0x10) {
            level = 0xF;
        }
    } else {
        level = 0;
    }
    g_SoundScale.scale = (level << 7) / 15;
}

void SetStereoOutput(void) { g_StereoOutput = 1; SetCdMixPreset(0); SsSetStereo(); }

void SetMonoOutput(void) { g_StereoOutput = 0; SetCdMixPreset(1); SsSetMono(); }
