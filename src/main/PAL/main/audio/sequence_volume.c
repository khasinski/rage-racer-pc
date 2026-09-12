#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/cd.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    SEQUENCE_VOLUME_AT_MAX_SETTING = 114,
};

void SetSequenceVolume(s32 volume) {
    volume = ClampVoiceVolume(volume);
    g_SeqVolume = volume;
    SsSeqSetVol((s16)g_SeqHandle, (s16)volume, (s16)volume);
}

static s32 SequenceVolumeForSetting(s32 setting) {
    setting = ClampAudioSetting(setting);
    return setting * SEQUENCE_VOLUME_AT_MAX_SETTING / AUDIO_SETTING_MAX;
}

void RefreshSequenceVolumeScale(void) {
    SetSequenceVolume(SequenceVolumeForSetting(g_SeqVolumeSetting));
}

void SetSequenceVolumeSetting(s32 setting) {
    setting = ClampAudioSetting(setting);
    g_SeqVolumeSetting = setting;
    SetCdVolumeSetting(setting);
    SetSequenceVolume(SequenceVolumeForSetting(setting));
}
