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

s32 g_CarSoundVolumeScales[CAR_SOUND_VOLUME_SCALE_COUNT] = {
    20, 21, 22, 23, 21, 22, 23, 22, 23, 26, 27, 28, 29, 30, 50, 52, 54, 50,
    52, 54, 52, 42, 44, 28, 28, 29, 30, 31, 30, 26, 46, 80
};
IndexedEffect g_IndexedEffects[AUDIO_INDEXED_EFFECT_COUNT] = {
    {14, 0, 64},
    {14, 0, 64},
    {16, 0, 90},
};
SoundModeEntry g_SoundModes[AUDIO_SOUND_MODE_COUNT] = {
    {2, 40, {{18, 0}, {18, 1}}},
    {2, 80, {{19, 0}, {19, 1}}},
    {2, 55, {{20, 0}, {20, 0}}},
    {2, 55, {{21, 0}, {21, 0}}},
};
s16 g_SoundSlotTone[ENGINE_SOUND_SLOT_COUNT][ENGINE_SOUND_BANK_COUNT] = {
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
Audio g_Audio;
MusicChannel g_MusicChannels[AUDIO_MUSIC_CHANNEL_COUNT];
EffectVoice g_EffectVoices[AUDIO_EFFECT_VOICE_COUNT];
