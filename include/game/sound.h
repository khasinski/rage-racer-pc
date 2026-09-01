#ifndef GAME_SOUND_H
#define GAME_SOUND_H

#include "common.h"

/* Volume-scale table at g_SoundScale. */
typedef struct SoundScale {
    s32 scale;
    s16 vabIds[8];
} SoundScale;

/*
 * Volume-scale record at 0x801E6CA4. Its `.scale` word is the master scale
 * applied to every sound-effect voice volume, 0..0x80: SetEffectVolumeScale
 * clamps into that range and the voice code multiplies a cue's nominal volume
 * by it before writing the SPU. `.vabIds` is the VAB id table at 0x801E6CA8.
 */
extern SoundScale g_SoundScale;

/*
 * Shared sound work area at 0x801E6D00..0x801E6DA8, three contiguous regions:
 *   0x6D00  MusicChannel[2]  stride 0x18
 *   0x6D30  EffectVoice[4]   stride 0x14 (hardware voices 10..13)
 *   0x6D80  scalar control block (reverb depth / volume scale / flags)
 */

/* Music / sound-mode channel; `left` and `right` are also read as their low
 * halves. Reset to left=right=-1, mode=1, vols=0. */
typedef union MusicChannelValue {
    s32 value;
    s16 half[2];
} MusicChannelValue;

typedef struct MusicChannel {
    MusicChannelValue left; /* +0x00 current left/tone value */
    MusicChannelValue right; /* +0x04 current right value */
    s32 mode;      /* +0x08 state/mode 0/1/2/-1 */
    s32 reserved;  /* +0x0C unused                              */
    s32 volLeft;  /* +0x10 scaled left volume */
    s32 volRight; /* +0x14 scaled right volume */
} MusicChannel; /* sizeof 0x18 */

extern MusicChannel g_MusicChannels[];

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

typedef struct EffectVoice {
    EffectVoiceNote note; /* +0x00 note/detune base */
    s32 tone;      /* +0x04 tone */
    s32 state;     /* +0x08 state 0/1/2/-1 */
    EffectVoicePitch pitch; /* +0x0C pitch */
    s32 volume;    /* +0x10 volume */
} EffectVoice; /* sizeof 0x14 */

extern EffectVoice g_EffectVoices[];

/* Scalar control block at 0x6D80. Retail addresses these individually by
 * symbol, never base+index, so they stay independent externs. */
extern s32 g_ReverbType; /* +0x00 */
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
extern s32 g_SeqVolumeFadeStep; /* step added to g_SeqVolume each frame; -4 while fading out */
extern s32 g_PrizeCountStep; /* +0x20 */

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

extern IndexedEffect g_IndexedEffects[];

typedef struct SoundModeSlot {
    s32 left;
    s32 right;
} SoundModeSlot;

typedef struct SoundModeEntry {
    s32 count;
    s32 factor;
    SoundModeSlot slots[2];
} SoundModeEntry;

extern SoundModeEntry g_SoundModes[];

/*
 * Pre-race BGM picker (scene 0xA, left/right on the pad). Per-file types.
 *   g_BgmSelection    g_BgmSelection  0 = shuffle, else track + 1; saved
 *   g_BgmShuffleOrder g_BgmShuffleOrder  the shuffle bag ShuffleBgmOrder refills
 *   g_BgmShuffleIndex g_BgmShuffleIndex  cursor into it, wraps at g_BgmTrackCount
 *   g_BgmTrack        g_BgmTrack  the chosen track; RequestCdTrack(n + 3)
 */


#endif
