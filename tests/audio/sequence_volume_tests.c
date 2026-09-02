#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>

s32 g_SeqVolumeSetting;
SequenceHandle g_SeqHandle;
s32 g_SeqVolume;

static s32 s_appliedVolume;
static s32 s_outputLeft;
static s32 s_outputRight;
static s32 s_cdVolumeSetting;
static s32 s_failures;

void SsSeqSetVol(short sequence, short left, short right) {
    (void)sequence;
    s_outputLeft = left;
    s_outputRight = right;
}
void SetCdVolumeSetting(s32 setting) { s_cdVolumeSetting = setting; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void CheckAppliedVolume(s32 expected, const char *label) {
    s_appliedVolume = g_SeqVolume;
    Check(s_appliedVolume == expected && s_outputLeft == expected &&
              s_outputRight == expected,
          label);
}

int main(void) {
    g_SeqVolumeSetting = 5;
    RefreshSequenceVolumeScale();
    Check(g_SeqVolumeSetting == 5,
          "refresh preserves the stored sequence setting");
    CheckAppliedVolume(38, "refresh applies volume to state and sequence");

    SetSequenceVolumeSetting(7);
    Check(g_SeqVolumeSetting == 7,
          "setter stores an intermediate sequence setting");
    CheckAppliedVolume(53, "setter applies intermediate sequence volume");
    Check(s_cdVolumeSetting == 7,
          "setter applies intermediate CD volume setting");

    SetSequenceVolumeSetting(AUDIO_SETTING_MAX);
    Check(g_SeqVolumeSetting == AUDIO_SETTING_MAX,
          "setter stores the maximum sequence setting");
    CheckAppliedVolume(114, "maximum setting maps to sequence volume maximum");

    if (s_failures != 0) return 1;
    puts("sequence volume refresh and setter share the same scale");
    return 0;
}
