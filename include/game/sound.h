#ifndef GAME_SOUND_H
#define GAME_SOUND_H

#include "common.h"

enum {
    AUDIO_MUSIC_CHANNEL_COUNT = 2,
    AUDIO_EFFECT_VOICE_COUNT = 4,
    AUDIO_INDEXED_EFFECT_COUNT = 3,
    AUDIO_SOUND_MODE_COUNT = 4,
};

typedef struct SoundScale {
    s32 scale;
    s16 vabIds[8];
} SoundScale;

/* Master effect-volume scale plus the libsnd id of each loaded VAB. */
extern SoundScale g_SoundScale;

/* Music / sound-mode channel; `left` and `right` are also read as their low
 * halves. Reset to left=right=-1, mode=idle, vols=0. */
typedef union MusicChannelValue {
    s32 value;
    s16 half[2];
} MusicChannelValue;

typedef enum MusicChannelState {
    MUSIC_CHANNEL_IDLE = -1,
    MUSIC_CHANNEL_START,
    MUSIC_CHANNEL_STOP,
    MUSIC_CHANNEL_UPDATE,
} MusicChannelState;

typedef struct MusicChannel {
    MusicChannelValue left; /* +0x00 current left/tone value */
    MusicChannelValue right; /* +0x04 current right value */
    MusicChannelState mode; /* +0x08 */
    s32 reserved;  /* +0x0C unused                              */
    s32 volLeft;  /* +0x10 scaled left volume */
    s32 volRight; /* +0x14 scaled right volume */
} MusicChannel; /* sizeof 0x18 */

_Static_assert(sizeof(MusicChannelState) == sizeof(s32),
               "music channel state ABI changed");
_Static_assert(sizeof(MusicChannel) == 0x18,
               "music channel record ABI changed");

extern MusicChannel g_MusicChannels[AUDIO_MUSIC_CHANNEL_COUNT];

/* Effect voice, 4 elements for hardware voices 10..13. SetPitchedSoundCue walks it
 * with a pointer to `.state`. */
typedef union EffectVoicePitch {
    s32 value;
    struct {
        u16 fraction;
        u16 upper;
    } half;
} EffectVoicePitch;

typedef union EffectVoiceNote {
    s32 value;
    struct {
        s16 value;
        s16 upper;
    } half;
} EffectVoiceNote;

typedef enum EffectVoiceState {
    EFFECT_VOICE_IDLE = -1,
    EFFECT_VOICE_START,
    EFFECT_VOICE_STOP,
    EFFECT_VOICE_UPDATE,
} EffectVoiceState;

typedef struct EffectVoice {
    EffectVoiceNote note; /* +0x00 note/detune base */
    s32 tone;      /* +0x04 tone */
    EffectVoiceState state; /* +0x08 */
    EffectVoicePitch pitch; /* +0x0C pitch */
    s32 volume;    /* +0x10 volume */
} EffectVoice; /* sizeof 0x14 */

_Static_assert(sizeof(EffectVoiceState) == sizeof(s32),
               "effect voice state ABI changed");
_Static_assert(sizeof(EffectVoice) == 0x14,
               "effect voice record ABI changed");

extern EffectVoice g_EffectVoices[AUDIO_EFFECT_VOICE_COUNT];

/* Runtime reverb and sequence controls shared by the audio update paths. */
extern s32 g_ReverbDepthL; /* reverb depth left  */
extern s32 g_ReverbDepthR; /* reverb depth right */
/* Per-frame step added to g_ReverbDepthL/R by UpdateSequenceFadeOut; -3
 * while a BGM fade-out runs, 0 when it has finished. */
extern s32 g_ReverbFadeStep;
typedef union SequenceHandle {
    s32 storage;
    s16 value;
} SequenceHandle;

extern SequenceHandle g_SeqHandle;
extern s32 g_SeqVolume; /* current SEQ volume, also read as s16 */
extern s32 g_SeqVolumeSetting; /* 0..15 OPTIONS level; volume = n * 114 / 15 */
/* Step added to g_SeqVolume each frame; -4 while fading out. */
extern s32 g_SeqVolumeFadeStep;
/* Per-slot engine tone, one entry per bank; a slot is re-cued when its two
 * banks disagree. The old g_SoundSlotToneBank1 symbol (g_SoundSlotToneBank1) is [i][1]
 * of this table. Six slots. */
extern s16 g_SoundSlotTone[][2];

/*
 * Indexed effect table in rodata at g_IndexedEffects: three entries, twelve bytes
 * each, selected by SetIndexedEffectVoice (index clamped to 0..2). The old
 * g_IndexedEffectVolumes symbol (g_IndexedEffectBaseVolumes) is g_IndexedEffects + 8, the third word
 * of the same element, which is why both were indexed by the same i * 12.
 * Retail data: { 14, 0, 64 }, { 14, 0, 64 }, { 16, 0, 90 }.
 */
typedef struct IndexedEffect {
    s32 tone;
    s32 unused;
    s32 volume;
} IndexedEffect; /* sizeof 0xC */

extern IndexedEffect g_IndexedEffects[AUDIO_INDEXED_EFFECT_COUNT];

typedef struct SoundModeSlot {
    s32 left;
    s32 right;
} SoundModeSlot;

typedef struct SoundModeEntry {
    s32 count;
    s32 factor;
    SoundModeSlot slots[2];
} SoundModeEntry;

extern SoundModeEntry g_SoundModes[AUDIO_SOUND_MODE_COUNT];

/*
 * Pre-race BGM picker (scene 0xA, left/right on the pad). Per-file types.
 *   g_BgmSelection    g_BgmSelection  0 = shuffle, else track + 1; saved
 *   g_BgmShuffleOrder g_BgmShuffleOrder  the shuffle bag ShuffleBgmOrder refills
 *   g_BgmShuffleIndex g_BgmShuffleIndex  cursor into it, wraps at g_BgmTrackCount
 *   g_BgmTrack        g_BgmTrack  the chosen track; RequestCdTrack(n + 3)
 */


#endif
