#include "game/audio.h"
#include "game/cd.h"
#include "game/sound.h"
#include "psyq/snd.h"

void SetLoadedTableVolumeScale(s32 scale) {
    scale = ClampVoiceVolume(scale);
    g_EngineSoundState.volumeScale = scale;
}

/* Set the effect master volume scale (g_SoundScale.scale) from a
 * 0..15 level, mapping it onto the 0..0x80 fixed-point scale used by the
 * effect-voice volume math. */
void SetEffectVolumeSetting(s32 level) {
    level = ClampAudioSetting(level);
    g_SoundScale.scale = (level << 7) / AUDIO_SETTING_MAX;
}

static void ApplyOutputMode(s32 mono) {
    g_StereoOutput = mono == 0;
    SetCdMixPreset(mono != 0);
    if (mono == 0) {
        SsSetStereo();
    } else {
        SsSetMono();
    }
}

void ApplyAudioSettings(void) {
    SetSequenceVolumeSetting(g_BgmVolumeSetting);
    SetEffectVolumeSetting(g_SfxVolumeSetting);
    ApplyOutputMode(g_MonoOutput);
}
