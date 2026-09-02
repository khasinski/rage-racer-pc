#include "game/audio.h"
#include "game/sound.h"

enum {
    SEQUENCE_VOLUME_AT_MAX_SETTING = 114,
};

void SetSequenceVolume(s32 volume) {
    g_SeqVolume = volume;
    SsSeqSetVol(g_SeqHandle.value, (s16)volume, (s16)volume);
}

static s32 SequenceVolumeForSetting(s32 setting) {
    return setting * SEQUENCE_VOLUME_AT_MAX_SETTING / AUDIO_SETTING_MAX;
}

void RefreshSequenceVolumeScale(void) {
    SetSequenceVolume(SequenceVolumeForSetting(g_SeqVolumeSetting));
}

void SetSequenceVolumeScale(s32 setting) {
    g_SeqVolumeSetting = setting;
    SetSequenceVolume(SequenceVolumeForSetting(setting));
}
