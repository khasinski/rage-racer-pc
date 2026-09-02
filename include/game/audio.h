#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

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

void SetSoundSlotTone(s32 slot, s32 bend, s32 volume, s32 toneIndex, u16 vabSlot);
void SsSeqPlay(short sequence, char playMode, short loopCount);
void SsSeqStop(short sequence);
void SsSeqSetVol(short sequence, short left, short right);

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


/*
 * The libsnd VAB ids of the loaded banks, one per bank slot.
 * StartAudioSlotLoad fills it while each VAB is imported; every key-on passes
 * an element as the vabId argument of SsUtKeyOnV / func_80078130
 * (`g_VabIds[slot]`), and the callers that only ever use the first bank read
 * `g_VabIds[0]`.
 */
extern s16 g_VabIds[];

void SetSequenceVolume(s32 volume);
void RefreshSequenceVolumeScale(void);
void PlaySequence(void);
void StartSequenceFadeOut(void);
void UpdateSequenceFadeOut(void);
void ApplyDuckedSequenceAudio(void);
void ApplyCurrentSequenceAudio(void);
/* Service libsnd's 60 Hz sequence clock from the game frame loop. */
void TickSequenceAudio(void);
void SetReverbDepth(s32 left, s32 right);
void SetReverbPreset(s32 type, s32 left, s32 right);
void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot);
void SetSoundSlotVoicesEnabled(s32 enabled);
void InitSoundRuntime(void);
s32 CloseLoadedAudioSlots(void);
void SetLoadedTableVolumeScale(s32 scale);
void SetSequenceVolumeSetting(s32 setting);
/* The effect-side twin of SetSequenceVolumeSetting: clamps the 0..15
 * option-screen level and scales it onto g_SoundScale.scale's 0..0x80 range. */
void SetEffectVolumeSetting(s32 setting);
void SetStereoOutput(void);
void SetMonoOutput(void);
/* Push all three saved audio settings (BGM level, SFX level, mono/stereo) into
 * the sound runtime; run at boot and again after a memory-card load. */
void ApplyAudioSettings(void);
void LoadAudioParameterTable(const u16 *table);
/* Open audio slot `slot` on a VAB header/body pair and optional tone table. */
s32 StartAudioSlotLoad(s32 slot, u8 *header, u8 *body, u16 *table);
s32 PollAudioSlotLoad(void);
void SetPanVoiceTargetVolume(s32 left, s32 right);
void ApplyPanVoiceVolume(void);
void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume);
void UpdateIndexedEffectVoice(void);
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
void ForcePanVoiceEnabled(s32 enabled);
void ForceIndexedEffectVoiceEnabled(s32 enabled);
s32 InterpolateAudioParameter(s32 param, s32 position, s32 bank);
void UpdateLoadedAudioVoices(s32 position, s32 bank);
void InitEffectVoiceRuntime(void);
void ForceBasicEffectVoicesEnabled(s32 enabled);
void ForcePitchEffectVoicesEnabled(s32 enabled);
void ForceSoundSlotVoicePlayback(s32 enabled);
void ForceAllEffectVoicesEnabled(s32 enabled);
s32 OpenSequenceAudioSlot(u8 *vabHeader, u8 *vabBody, void *seqData);
void SetDefaultReverbDepth(void);
void InitSequenceAudio(void);
int CloseSequenceAudioSlot(void);

/* Declared identically by 8 translation units before this
 * header carried them. */

enum {
    ENGINE_SOUND_BANK_COUNT = 2,
    ENGINE_SOUND_PARAMETER_COUNT = 12,
    ENGINE_SOUND_CURVE_POINT_COUNT = 9,
    ENGINE_SOUND_SLOT_COUNT = 6,
    ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT =
        ENGINE_SOUND_BANK_COUNT * ENGINE_SOUND_PARAMETER_COUNT *
            ENGINE_SOUND_CURVE_POINT_COUNT * 2 +
        1 + ENGINE_SOUND_BANK_COUNT * ENGINE_SOUND_SLOT_COUNT + 1,
};

typedef struct EngineSoundCurveRow {
    s32 positions[ENGINE_SOUND_CURVE_POINT_COUNT];
    s32 values[ENGINE_SOUND_CURVE_POINT_COUNT];
} EngineSoundCurveRow;

typedef struct EngineSoundState {
    s32 position;
    s32 bank;
    /* Retail +0x08 flag, written after the engine VAB transfer. No current C
     * reader, but retaining it preserves every following runtime offset. */
    s32 extraVabLoaded;
    s32 maxRpm;
    s32 slotActive[ENGINE_SOUND_SLOT_COUNT];
    s32 volumeScale;
} EngineSoundState;

_Static_assert(sizeof(EngineSoundState) == 44,
               "engine sound runtime ABI changed");
_Static_assert(__builtin_offsetof(EngineSoundState, maxRpm) == 12,
               "engine max-RPM offset changed");
_Static_assert(__builtin_offsetof(EngineSoundState, slotActive) == 16,
               "engine sound slot offset changed");
_Static_assert(__builtin_offsetof(EngineSoundState, volumeScale) == 40,
               "engine volume-scale offset changed");

extern EngineSoundCurveRow
    g_EngineSoundCurves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];
extern EngineSoundState g_EngineSoundState;

/* Declared identically by 62 translation units before this
 * header carried them. */

extern s32 g_SpecialVoiceBits4;
extern const char g_MsgTooManyVoices[];
extern s32 g_ActiveSpecialCue;
extern s32 g_AudioLoadSlot;
extern s32 g_AudioLoadedSlotMask;
extern s32 g_CarSoundVolumeScales[];
extern EffectCueBank g_EffectCueTable[3];
extern s32 g_IndexedEffectIndex;
extern s32 g_IndexedEffectIndexPrev;
extern s32 g_IndexedEffectPitch;
extern s32 g_IndexedEffectVolume;
extern s32 g_LastSpecialCueRequest;
extern char g_MsgSeqVabOpenHeadError[];
extern char g_MsgSeqVabTransBodyError[];
extern char g_MsgVabOpenHeadError[];
extern char g_MsgVabTransBodyError[];
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

extern SoundCueParams g_SoundCueParams[];
extern SoundCueParams g_SoundCueParams2[];
extern s32 g_SpecialCueVoiceA;
extern s32 g_SpecialCueVoiceB;
extern s32 g_SpecialVoiceBits[];
extern s32 g_StereoOutput;
/*
 * SPU addresses for the four VAB slots.  Slots 0..2 are loaded with
 * g_VabIds[slot] / g_VabSpuAddress[slot]; slot 3 is the extra bank, which had
 * both of its entries split off under their own names.
 */
extern s32 g_VabSpuAddress[];
extern s32 g_VabTransferDone;

/* The BIOS exit service does not return after a fatal asset-loading error. */
void BiosExit(s32 code) __attribute__((noreturn));
void UpdateBasicEffectVoices(void);
void UpdateEffectVoiceStates(void);

#endif
