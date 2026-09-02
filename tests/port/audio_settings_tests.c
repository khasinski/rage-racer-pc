#include "common.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/sound.h"

#include <stdio.h>

EngineSoundState g_EngineSoundState;
SoundScale g_SoundScale;
s32 g_StereoOutput;
s32 g_BgmVolumeSetting;
s32 g_SfxVolumeSetting;
s32 g_MonoOutput;
s32 g_SeqVolumeSetting;
s32 g_SeqVolume;
SequenceHandle g_SeqHandle;

static s32 s_cdVolumeSetting;
static s32 s_sequenceVolume;
static s32 s_cdMixPreset;
static s32 s_stereoCalls;
static s32 s_monoCalls;
static s32 s_failures;

void SetCdVolumeSetting(s32 level) { s_cdVolumeSetting = level; }
void SsSeqSetVol(short sequence, short left, short right) {
    (void)sequence;
    (void)right;
    s_sequenceVolume = left;
}
void SetCdMixPreset(s32 preset) { s_cdMixPreset = preset; }
void SsSetStereo(void) { s_stereoCalls++; }
void SsSetMono(void) { s_monoCalls++; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestAudioSettingClamp(void) {
    Check(ClampAudioSetting(-1) == 0, "negative audio setting clamps to zero");
    Check(ClampAudioSetting(0) == 0, "zero audio setting is preserved");
    Check(ClampAudioSetting(7) == 7, "intermediate audio setting is preserved");
    Check(ClampAudioSetting(15) == 15, "maximum audio setting is preserved");
    Check(ClampAudioSetting(16) == 15, "high audio setting clamps to maximum");
}

static void TestVolumeSettings(void) {
    SetSequenceVolumeSetting(-4);
    Check(s_cdVolumeSetting == 0 && g_SeqVolumeSetting == 0 &&
              s_sequenceVolume == 0,
          "sequence setting shares its clamped zero");
    SetSequenceVolumeSetting(99);
    Check(s_cdVolumeSetting == 15 && g_SeqVolumeSetting == 15 &&
              s_sequenceVolume == 114,
          "sequence setting shares its clamped maximum");

    SetEffectVolumeSetting(-1);
    Check(g_SoundScale.scale == 0, "negative effect setting mutes effects");
    SetEffectVolumeSetting(7);
    Check(g_SoundScale.scale == 59,
          "intermediate effect setting uses fixed-point scale");
    SetEffectVolumeSetting(15);
    Check(g_SoundScale.scale == 128,
          "maximum effect setting maps to full scale");

    SetLoadedTableVolumeScale(-1);
    Check(g_EngineSoundState.volumeScale == 0,
          "negative loaded table scale clamps to zero");
    SetLoadedTableVolumeScale(129);
    Check(g_EngineSoundState.volumeScale == 128,
          "loaded table scale clamps to voice maximum");
}

static void TestOutputMode(void) {
    g_MonoOutput = 0;
    ApplyAudioSettings();
    Check(g_StereoOutput == 1 && s_cdMixPreset == 0 && s_stereoCalls == 1,
          "stereo mode updates game, CD, and SPU state");
    g_MonoOutput = 1;
    ApplyAudioSettings();
    Check(g_StereoOutput == 0 && s_cdMixPreset == 1 && s_monoCalls == 1,
          "mono mode updates game, CD, and SPU state");
}

static void TestApplyingSavedSettings(void) {
    g_BgmVolumeSetting = 6;
    g_SfxVolumeSetting = 9;
    g_MonoOutput = 0;
    ApplyAudioSettings();
    Check(s_cdVolumeSetting == 6 && g_SeqVolumeSetting == 6 &&
              s_sequenceVolume == 45,
          "saved BGM setting reaches CD and sequence output");
    Check(g_SoundScale.scale == 76 && g_StereoOutput == 1,
          "saved SFX and stereo settings reach the runtime");

    g_BgmVolumeSetting = 99;
    g_SfxVolumeSetting = -1;
    g_MonoOutput = 1;
    ApplyAudioSettings();
    Check(s_cdVolumeSetting == 15 && g_SoundScale.scale == 0,
          "saved volume settings are clamped while applying");
    Check(g_StereoOutput == 0,
          "saved mono setting reaches the runtime");
}

int main(void) {
    TestAudioSettingClamp();
    TestVolumeSettings();
    TestOutputMode();
    TestApplyingSavedSettings();

    if (s_failures != 0) return 1;
    puts("audio settings clamp and update every output layer");
    return 0;
}
