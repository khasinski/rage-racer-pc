#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>

s32 g_ReverbDepthL;
s32 g_ReverbDepthR;
s32 g_ReverbFadeStep;
s32 g_ReverbType;
s32 g_SeqVolumeFadeStep;
s32 g_SceneId;
SoundScale g_SoundScale;
s16 g_SoundSlotTone[6][2];

static s32 s_frameHz = 50;
static s32 s_sequenceTicks;
static s32 s_damperSteps;
static s32 s_fadeUpdates;
static s32 s_reverbOffCalls;
static s32 s_reverbOnCalls;
static s32 s_reverbType;
static s32 s_reverbLeft;
static s32 s_reverbRight;
static long s_voice;
static long s_vabId;
static long s_program;
static long s_note;
static s32 s_failures;

int TimingBaseHz(void) { return s_frameHz; }
void SsSeqCalledTbyT(void) { s_sequenceTicks++; }
void SpuVmDamperStep(void) { s_damperSteps++; }
void UpdateSequenceFadeOut(void) { s_fadeUpdates++; }
void SsUtSetReverbDepth(long left, long right) {
    s_reverbLeft = (s32)left;
    s_reverbRight = (s32)right;
}
short SsUtSetReverbType(short type) {
    s_reverbType = type;
    return type;
}
void SsUtReverbOn(void) { s_reverbOnCalls++; }
void SsUtReverbOff(void) { s_reverbOffCalls++; }
long SsUtKeyOnV(long voice, long vabId, long program, long tone, long note,
                long fine, long volLeft, long volRight) {
    (void)tone;
    (void)fine;
    (void)volLeft;
    (void)volRight;
    s_voice = voice;
    s_vabId = vabId;
    s_program = program;
    s_note = note;
    return voice;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestSequenceTicking(void) {
    s32 frame;

    g_SceneId = 0;
    g_SeqVolumeFadeStep = -4;
    for (frame = 0; frame < 5; frame++) TickSequenceAudio();
    Check(s_sequenceTicks == 6,
          "PAL frames service the sixty-hertz sequence clock");
    Check(s_damperSteps == 5 && s_fadeUpdates == 5,
          "normal audio frames flush voices and advance fades");

    g_SceneId = 0xC;
    TickSequenceAudio();
    Check(s_sequenceTicks == 6 && s_fadeUpdates == 5 && s_damperSteps == 6,
          "sound-mode scene only services the voice damper");

    g_SceneId = 0;
    g_SeqVolumeFadeStep = 0;
    s_frameHz = 60;
    for (frame = 0; frame < 3; frame++) TickSequenceAudio();
    Check(s_sequenceTicks == 9 && s_fadeUpdates == 5 && s_damperSteps == 9,
          "NTSC frames service one sequence tick without an inactive fade");
}

static void TestReverbDepth(void) {
    SetReverbDepth(-1, 128);
    Check(g_ReverbDepthL == 0 && g_ReverbDepthR == 127 &&
              s_reverbLeft == 0 && s_reverbRight == 127,
          "reverb depth clamps game and SPU values");

    SetDefaultReverbDepth();
    Check(g_ReverbDepthL == 40 && g_ReverbDepthR == 40,
          "default reverb depth uses the game preset");
}

static void TestReverbPresets(void) {
    s_reverbOffCalls = 0;
    s_reverbOnCalls = 0;
    SetReverbPreset(9, -3, 200);
    Check(s_reverbOffCalls == 1 && s_reverbOnCalls == 1 &&
              s_reverbType == 9 && g_ReverbType == 9,
          "valid reverb preset restarts the SPU effect");
    Check(g_ReverbDepthL == 0 && g_ReverbDepthR == 127,
          "reverb preset applies clamped depths");

    SetReverbPreset(0, 50, 60);
    Check(s_reverbOffCalls == 2 && s_reverbOnCalls == 1 &&
              g_ReverbType == 0 && g_ReverbDepthL == 0 &&
              g_ReverbDepthR == 0,
          "zero preset disables reverb");
    SetReverbPreset(10, 50, 60);
    Check(s_reverbOffCalls == 3 && s_reverbOnCalls == 1 &&
              g_ReverbType == 0,
          "preset above the supported range disables reverb");
    SetReverbPreset(-1, 50, 60);
    Check(s_reverbOffCalls == 4 && s_reverbOnCalls == 1 &&
              g_ReverbType == 0,
          "preset below the supported range disables reverb");
}

static void TestSoundSlotVoice(void) {
    g_SoundScale.vabIds[1] = 23;
    g_SoundSlotTone[2][1] = 71;
    PlaySoundSlotVoice(2, 1, 1);
    Check(s_voice == 16 && s_vabId == 23 && s_program == 71 && s_note == 60,
          "sound slot maps to its reserved hardware voice and program");

    s_voice = -1;
    PlaySoundSlotVoice(-1, 1, 1);
    PlaySoundSlotVoice(2, ENGINE_SOUND_BANK_COUNT, 1);
    PlaySoundSlotVoice(2, 1, AUDIO_SLOT_COUNT);
    Check(s_voice == -1, "invalid sound slot routes are ignored");
}

int main(void) {
    TestSequenceTicking();
    TestReverbDepth();
    TestReverbPresets();
    TestSoundSlotVoice();

    if (s_failures != 0) return 1;
    puts("audio runtime clocks sequences and applies reverb and slot voices");
    return 0;
}
