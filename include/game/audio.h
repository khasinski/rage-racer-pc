#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <stddef.h>

#include "common.h"

typedef struct EffectCueProgram {
    s32 note;
    s32 tone;
} EffectCueProgram;

typedef enum AudioSlotId {
    AUDIO_SLOT_MAIN_CUES,
    AUDIO_SLOT_SEQUENCE,
    AUDIO_SLOT_RACE_CUES,
    AUDIO_SLOT_ENGINE,
    AUDIO_SLOT_COUNT,
} AudioSlotId;

enum {
    MAIN_SOUND_CUE_COUNT = 30,
    RACE_SOUND_CUE_COUNT = 70,
    EFFECT_CUE_BANK_COUNT = 3,
    SPECIAL_VOICE_BIT_COUNT = 6,
    CAR_SOUND_VOLUME_SCALE_COUNT = 32,
};


/*
 * The SPU takes a voice volume in 0..0x80, and a cue level in 0..0x7F. Both
 * clamps were written out in full at every call, twice over wherever the two
 * channels were done one after the other.
 */
static inline s32 ClampVoiceVolume(s32 volume) {
    if (volume < 0) {
        return 0;
    }
    return volume > 0x80 ? 0x80 : volume;
}

static inline s32 ClampCueLevel(s32 level) {
    if (level < 0) {
        return 0;
    }
    return level >= 0x80 ? 0x7F : level;
}

enum { AUDIO_SETTING_MAX = 15 };

extern s32 g_BgmVolumeSetting;
extern s32 g_SfxVolumeSetting;
extern s32 g_MonoOutput;

static inline s32 ClampAudioSetting(s32 setting) {
    if (setting < 0) {
        return 0;
    }
    return setting > AUDIO_SETTING_MAX ? AUDIO_SETTING_MAX : setting;
}

typedef struct EffectCueBankHeader {
    s32 voiceCount;
    s32 volumeScale;
} EffectCueBankHeader;

typedef struct EffectCueBank {
    s32 voiceCount;
    s32 volumeScale;
    EffectCueProgram programs[2];
} EffectCueBank;
void PlaySequence(void);
void StartSequenceFadeOut(void);
void ApplyDuckedSequenceAudio(void);
void ApplyCurrentSequenceAudio(void);
/* Service libsnd's 60 Hz sequence clock from the game frame loop. */
void TickSequenceAudio(void);
void SetReverbDepth(s32 left, s32 right);
void SetReverbPreset(s32 type, s32 left, s32 right);
void InitSoundRuntime(void);
void CloseLoadedAudioSlots(void);
void SetSequenceVolumeSetting(s32 setting);
/* The effect-side twin of SetSequenceVolumeSetting: clamps the 0..15
 * option-screen level and scales it onto g_SoundScale.scale's 0..0x80 range. */
void SetEffectVolumeSetting(s32 setting);
/* Push all three saved audio settings (BGM level, SFX level, mono/stereo) into
 * the sound runtime; run at boot and again after a memory-card load. */
void ApplyAudioSettings(void);
typedef struct AudioSlotAsset {
    u8 *vabHeader;
    size_t vabHeaderSize;
    u8 *vabBody;
    size_t vabBodySize;
    void *auxiliaryData;
    size_t auxiliarySize;
} AudioSlotAsset;

/* Validate and open one VAB plus either SEQ data or the engine parameter
 * table. Zero means an asynchronous transfer is still in progress, a
 * positive value means complete, and -1 reports malformed data or failure. */
s32 StartAudioSlotLoad(s32 slot, const AudioSlotAsset *asset);
s32 PollAudioSlotLoad(void);
void SetPanVoiceTargetVolume(s32 left, s32 right);
void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume);
/*
 * The two positional-cue setters. `cue` is an index into the shared 7-record
 * table at D_800126D0 (stride 0x18); each cue owns a fixed pair of voices, and a
 * call keys that pair on, updates it in place, or keys it off at volume 0.
 * Pitched drives EffectVoice D_801E6D30[4] with a 7.7 note; Stereo drives
 * D_801E6D00[2] with independent volumes.
 */
void SetPitchedSoundCue(s32 cue, s32 pitch, s32 volume);
void SetStereoSoundCue(s32 cue, s32 volLeft, s32 volRight);
void PlaySoundCue(s32 cue);
void UpdateLoadedAudioVoices(s32 position, s32 bank);
void InitEffectVoiceRuntime(void);
void ForceAllEffectVoicesEnabled(s32 enabled);
void SetDefaultReverbDepth(void);
void InitSequenceAudio(void);

enum {
    ENGINE_SOUND_BANK_COUNT = 2,
    ENGINE_SOUND_PARAMETER_COUNT = 12,
    ENGINE_SOUND_CURVE_POINT_COUNT = 9,
    ENGINE_SOUND_SLOT_COUNT = 6,
    ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT =
        ENGINE_SOUND_BANK_COUNT * ENGINE_SOUND_PARAMETER_COUNT *
            ENGINE_SOUND_CURVE_POINT_COUNT * 2 +
        1 + ENGINE_SOUND_BANK_COUNT * ENGINE_SOUND_SLOT_COUNT + 1,
    ENGINE_SOUND_PARAMETER_TABLE_SIZE =
        ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT * sizeof(u16),
};

typedef struct EngineSoundCurveRow {
    s32 positions[ENGINE_SOUND_CURVE_POINT_COUNT];
    s32 values[ENGINE_SOUND_CURVE_POINT_COUNT];
} EngineSoundCurveRow;

typedef struct EngineSoundState {
    s32 position;
    s32 bank;
    s32 maxRpm;
    s32 slotActive[ENGINE_SOUND_SLOT_COUNT];
    s32 volumeScale;
} EngineSoundState;

_Static_assert(sizeof(EngineSoundState) == 40,
               "engine sound runtime ABI changed");
_Static_assert(__builtin_offsetof(EngineSoundState, maxRpm) == 8,
               "engine max-RPM offset changed");
_Static_assert(__builtin_offsetof(EngineSoundState, slotActive) == 12,
               "engine sound slot offset changed");
_Static_assert(__builtin_offsetof(EngineSoundState, volumeScale) == 36,
               "engine volume-scale offset changed");

extern EngineSoundCurveRow
    g_EngineSoundCurves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];
extern EngineSoundState g_EngineSoundState;

extern const char g_MsgTooManyVoices[];
extern s32 g_ActiveSpecialCue;
extern s32 g_AudioLoadSlot;
extern s32 g_AudioLoadedSlotMask;
extern s32 g_CarSoundVolumeScales[CAR_SOUND_VOLUME_SCALE_COUNT];
extern EffectCueBank g_EffectCueTable[EFFECT_CUE_BANK_COUNT];
extern s32 g_IndexedEffectIndex;
extern s32 g_IndexedEffectIndexPrev;
extern s32 g_IndexedEffectPitch;
extern s32 g_IndexedEffectVolume;
extern s32 g_LastSpecialCueRequest;
extern const char g_MsgSeqVabOpenHeadError[];
extern const char g_MsgSeqVabTransBodyError[];
extern const char g_MsgVabOpenHeadError[];
extern const char g_MsgVabTransBodyError[];
extern s32 g_PanVoiceActive;
extern s32 g_PanVoiceVolumeL;
extern s32 g_PanVoiceVolumeR;
extern s32 g_SoundCueBank;
typedef struct SoundCueParams {
    s32 volume;
    s32 vab;
    s32 program;
    s32 toneA;
    s32 toneB;
    s32 reserved;
} SoundCueParams;

extern SoundCueParams g_SoundCueParams[MAIN_SOUND_CUE_COUNT];
extern SoundCueParams g_SoundCueParams2[RACE_SOUND_CUE_COUNT];
extern s32 g_SpecialVoiceBits[SPECIAL_VOICE_BIT_COUNT];
extern s32 g_StereoOutput;
/*
 * SPU addresses for the four VAB slots.  Slots 0..2 are loaded with
 * g_SoundScale.vabIds[slot] and g_VabSpuAddress[slot]; slot 3 is the engine
 * sound bank.
 */
extern s32 g_VabSpuAddress[AUDIO_SLOT_COUNT];
#endif
