#ifndef GAME_AUDIO_INTERNAL_H
#define GAME_AUDIO_INTERNAL_H

#include <stddef.h>

#include "common.h"

enum {
    BGM_PLAYABLE_TRACK_COUNT = 10,
    BGM_SHUFFLE_CAPACITY = 12,
};

extern s32 g_BgmShuffleIndex;
extern u8 g_BgmShuffleOrder[BGM_SHUFFLE_CAPACITY];

s32 OpenSequenceAudioSlot(u8 *vabHeader, u8 *vabBody, void *seqData);
s32 CloseSequenceAudioSlot(void);
void LoadAudioParameterTable(const void *data, size_t size);
void SetLoadedTableVolumeScale(s32 scale);
void SetSequenceVolume(s32 volume);
void RefreshSequenceVolumeScale(void);
void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot);
void SetSoundSlotVoicesEnabled(s32 enabled);
void ApplyPanVoiceVolume(void);
void UpdateIndexedEffectVoice(void);
void UpdateBasicEffectVoices(void);
void UpdateEffectVoiceStates(void);

s32 ClampBgmTrackCount(s32 trackCount);
s32 ClampBgmShuffleCount(s32 trackCount);
s32 BgmShuffleTrackAt(const u8 *shuffleOrder, s32 trackCount,
                      s32 shuffleIndex);

#endif
