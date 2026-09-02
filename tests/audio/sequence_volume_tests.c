#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>

s32 g_SeqVolumeSetting;

static s32 s_appliedVolume;
static s32 s_failures;

void SetSequenceVolume(s32 volume) { s_appliedVolume = volume; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

int main(void) {
    g_SeqVolumeSetting = 5;
    RefreshSequenceVolumeScale();
    Check(g_SeqVolumeSetting == 5 && s_appliedVolume == 38,
          "refresh reapplies the stored setting without changing it");

    SetSequenceVolumeScale(7);
    Check(g_SeqVolumeSetting == 7 && s_appliedVolume == 53,
          "setter stores and applies an intermediate setting");

    SetSequenceVolumeScale(AUDIO_SETTING_MAX);
    Check(g_SeqVolumeSetting == AUDIO_SETTING_MAX && s_appliedVolume == 114,
          "maximum setting maps to the sequence volume maximum");

    if (s_failures != 0) return 1;
    puts("sequence volume refresh and setter share the same scale");
    return 0;
}
