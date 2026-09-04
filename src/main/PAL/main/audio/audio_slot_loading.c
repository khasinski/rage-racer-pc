#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

enum {
    VAB_HEADER_SIZE = 32,
    VAB_VERSION_OFFSET = 4,
    VAB_PROGRAM_COUNT_OFFSET = 18,
    VAB_SAMPLE_COUNT_OFFSET = 22,
    VAB_PROGRAM_ATTRIBUTE_SIZE = 16,
    VAB_TONE_ATTRIBUTE_SIZE = 32,
    VAB_OLD_PROGRAM_LIMIT = 64,
    VAB_NEW_PROGRAM_LIMIT = 128,
    VAB_TONES_PER_PROGRAM = 16,
    VAB_LENGTH_TABLE_ENTRIES = 256,
    SEQUENCE_HEADER_SIZE = 15,
};

static u16 ReadLittleEndianU16(const u8 *bytes) {
    return (u16)(bytes[0] | (u16)bytes[1] << 8);
}

static u32 ReadLittleEndianU32(const u8 *bytes) {
    return (u32)bytes[0] | (u32)bytes[1] << 8 | (u32)bytes[2] << 16 |
           (u32)bytes[3] << 24;
}

static s32 IsValidSequenceAsset(const void *data, size_t size) {
    const u8 *bytes = data;

    return bytes != NULL && size >= SEQUENCE_HEADER_SIZE + 1 &&
           (bytes[0] == 'S' || bytes[0] == 'p') && bytes[7] == 1 &&
           (bytes[8] != 0 || bytes[9] != 0) &&
           (bytes[10] != 0 || bytes[11] != 0 || bytes[12] != 0);
}

static s32 IsValidVabAsset(const AudioSlotAsset *asset) {
    const u8 *header;
    const u8 *lengths;
    size_t requiredHeaderSize;
    size_t requiredBodySize = 0;
    u32 version;
    u16 programCount;
    u16 sampleCount;
    s32 programLimit;
    u32 i;

    if (asset == NULL || asset->vabHeader == NULL ||
        asset->vabBody == NULL || asset->vabHeaderSize < VAB_HEADER_SIZE) {
        return 0;
    }
    header = asset->vabHeader;
    if (header[0] != 'p' || header[1] != 'B' ||
        header[2] != 'A' || header[3] != 'V') {
        return 0;
    }
    version = ReadLittleEndianU32(header + VAB_VERSION_OFFSET);
    programCount = ReadLittleEndianU16(header + VAB_PROGRAM_COUNT_OFFSET);
    sampleCount = ReadLittleEndianU16(header + VAB_SAMPLE_COUNT_OFFSET);
    if (sampleCount >= VAB_LENGTH_TABLE_ENTRIES) {
        return 0;
    }

    programLimit = version >= 5
                       ? VAB_NEW_PROGRAM_LIMIT
                       : VAB_OLD_PROGRAM_LIMIT;
    if (programCount > programLimit) {
        return 0;
    }

    requiredHeaderSize = VAB_HEADER_SIZE +
                         (size_t)programLimit * VAB_PROGRAM_ATTRIBUTE_SIZE +
                         (size_t)programCount * VAB_TONES_PER_PROGRAM *
                             VAB_TONE_ATTRIBUTE_SIZE +
                         VAB_LENGTH_TABLE_ENTRIES * sizeof(u16);
    if (requiredHeaderSize > asset->vabHeaderSize) {
        return 0;
    }

    lengths = header + requiredHeaderSize -
              VAB_LENGTH_TABLE_ENTRIES * sizeof(u16);
    for (i = 0; i <= sampleCount; i++) {
        requiredBodySize += (size_t)ReadLittleEndianU16(lengths + i * 2) *
                            (version >= 5 ? 8u : 4u);
    }
    return requiredBodySize <= asset->vabBodySize;
}

static s32 TransferVabToSlot(s32 slot, u8 *header, u8 *body,
                             s32 spuAddress) {
    s16 openedVabId = SsVabOpenHeadSticky(header, -1, spuAddress);
    s16 vabId;

    if (openedVabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        return 0;
    }

    vabId = SsVabTransBody(body, openedVabId);
    if (vabId == -1) {
        SsVabClose(openedVabId);
        printf("%s", g_MsgVabTransBodyError);
        return 0;
    }

    g_SoundScale.vabIds[slot] = vabId;
    return 1;
}

static s32 StartEngineAudioSlotLoad(const AudioSlotAsset *asset) {
    if (asset->auxiliaryData != NULL &&
        asset->auxiliarySize < ENGINE_SOUND_PARAMETER_TABLE_SIZE) {
        return -1;
    }
    if (!TransferVabToSlot(AUDIO_SLOT_ENGINE, asset->vabHeader,
                           asset->vabBody,
                           g_VabSpuAddress[AUDIO_SLOT_ENGINE])) {
        return -1;
    }

    if (asset->auxiliaryData != NULL) {
        LoadAudioParameterTable(asset->auxiliaryData,
                                asset->auxiliarySize);
    }

    g_AudioLoadSlot = AUDIO_SLOT_ENGINE;
    return SsVabTransCompleted(0);
}

s32 StartAudioSlotLoad(s32 slot, const AudioSlotAsset *asset) {
    if (slot < 0 || slot >= AUDIO_SLOT_COUNT || !IsValidVabAsset(asset)) {
        return -1;
    }
    if (slot == AUDIO_SLOT_ENGINE) {
        return StartEngineAudioSlotLoad(asset);
    }
    if (slot == AUDIO_SLOT_SEQUENCE) {
        if (!IsValidSequenceAsset(asset->auxiliaryData,
                                  asset->auxiliarySize)) {
            return -1;
        }
        return OpenSequenceAudioSlot(asset->vabHeader, asset->vabBody,
                                     asset->auxiliaryData);
    }

    if (!TransferVabToSlot(slot, asset->vabHeader, asset->vabBody,
                           g_VabSpuAddress[slot])) {
        return -1;
    }

    g_AudioLoadSlot = slot;
    return SsVabTransCompleted(0);
}

s32 PollAudioSlotLoad(void) {
    s32 completed;
    s32 slot;

    completed = SsVabTransCompleted(0);
    if (completed != 0) {
        slot = g_AudioLoadSlot;
        g_AudioLoadSlot = -1;
        if ((u32)slot >= AUDIO_SLOT_COUNT) {
            return completed;
        }
        g_AudioLoadedSlotMask |= 1 << slot;

        if (slot == AUDIO_SLOT_MAIN_CUES) {
            g_SoundCueBank = 1;
        } else if (slot == AUDIO_SLOT_SEQUENCE) {
            g_SoundCueBank = slot;
        } else if (slot == AUDIO_SLOT_RACE_CUES ||
                   slot == AUDIO_SLOT_ENGINE) {
            g_SoundCueBank = 2;
        }
    }

    return completed;
}

static void CloseVabOnlyAudioSlot(s32 slot) {
    s32 bit;

    if (slot < 0 || slot >= AUDIO_SLOT_COUNT) {
        return;
    }

    bit = 1 << slot;

    if ((bit & g_AudioLoadedSlotMask) == 0) {
        return;
    }

    g_AudioLoadedSlotMask &= ~bit;
    SsUtSetReverbDepth(0, 0);
    _SsVmInit(0);
    SsVabClose(g_SoundScale.vabIds[slot]);
}

void CloseLoadedAudioSlots(void) {
    SpuVmDamperStep();
    CloseSequenceAudioSlot();
    CloseVabOnlyAudioSlot(AUDIO_SLOT_RACE_CUES);
    CloseVabOnlyAudioSlot(AUDIO_SLOT_ENGINE);
}
