/*
 * Retail state the sound code reads: the engine note, the sample and sequence
 * handles in play, the cues a race raises, and the output settings.
 *
 * The port drives audio through PsyZ, whose libsnd/libspu implementation owns
 * its internal state.  The obsolete retail work areas are intentionally not
 * reproduced here.
 */

#include <stddef.h>

#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

s32 g_CarSoundVolumeScales[CAR_SOUND_VOLUME_SCALE_COUNT]
    __attribute__((aligned(16))) = {
    20, 21, 22, 23, 21, 22, 23, 22, 23, 26, 27, 28, 29, 30, 50, 52, 54, 50,
    52, 54, 52, 42, 44, 28, 28, 29, 30, 31, 30, 26, 46, 80
};
const char g_MsgVabOpenHeadError[] __attribute__((aligned(16))) =
    "SsVabOpenHead Error\n";
const char g_MsgVabTransBodyError[] __attribute__((aligned(16))) =
    "SsVabTransBody Error\n";
IndexedEffect g_IndexedEffects[AUDIO_INDEXED_EFFECT_COUNT]
    __attribute__((aligned(16))) = {
    {14, 0, 64},
    {14, 0, 64},
    {16, 0, 90},
};
SoundModeEntry g_SoundModes[AUDIO_SOUND_MODE_COUNT]
    __attribute__((aligned(16))) = {
    {2, 40, {{18, 0}, {18, 1}}},
    {2, 80, {{19, 0}, {19, 1}}},
    {2, 55, {{20, 0}, {20, 0}}},
    {2, 55, {{21, 0}, {21, 0}}},
};
const char g_MsgTooManyVoices[16] __attribute__((aligned(16))) = "Too many voice\n";
const char g_MsgSeqVabOpenHeadError[] __attribute__((aligned(16))) =
    "SsVabOpenHead Error\n";
const char g_MsgSeqVabTransBodyError[] __attribute__((aligned(16))) =
    "SsVabTransBody Error\n";
s16 g_SoundSlotTone[ENGINE_SOUND_SLOT_COUNT][ENGINE_SOUND_BANK_COUNT]
    __attribute__((aligned(16))) = {
    {1, 1}, {2, 2}, {3, 3}, {5, 4}, {7, 6}, {8, 8},
};
s32 g_StereoOutput = 1;
s32 g_ActiveSpecialCue;
s32 g_LastSpecialCueRequest = 17;
s32 g_AudioLoadSlot;
EngineSoundCurveRow
    g_EngineSoundCurves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];
s32 g_MonoOutput;
s32 g_AudioLoadedSlotMask;
s32 g_SoundCueBank;
EngineSoundState g_EngineSoundState;
s32 g_PanVoiceVolumeL;
s32 g_PanVoiceVolumeR;
s32 g_PanVoiceActive;
s32 g_IndexedEffectIndex;
s32 g_IndexedEffectIndexPrev;
s32 g_IndexedEffectPitch;
s32 g_IndexedEffectVolume;
MusicChannel g_MusicChannels[AUDIO_MUSIC_CHANNEL_COUNT];
EffectVoice g_EffectVoices[AUDIO_EFFECT_VOICE_COUNT];
s32 g_ReverbDepthL;
s32 g_ReverbDepthR;
s32 g_ReverbFadeStep;
SequenceHandle g_SeqHandle;
s32 g_SeqVolume;
s32 g_SeqVolumeSetting;
s32 g_SeqVolumeFadeStep;
