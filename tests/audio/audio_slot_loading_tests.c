#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Audio g_Audio;
SoundScale g_SoundScale;
EngineSoundState g_EngineSoundState;
s32 g_VabSpuAddress[AUDIO_SLOT_COUNT];

static s16 s_openResult = 7;
static s16 s_bodyResult = 8;
static s16 s_completed = 1;
static s32 s_openAddress;
static u8 *s_openHeader;
static u8 *s_body;
static s32 s_tableCalls;
static const void *s_tableData;
static size_t s_tableSize;
static s32 s_closeVab;
static s32 s_reverbCalls;
static s32 s_vmInitCalls;
static s32 s_damperCalls;

short SsVabOpenHeadSticky(u8 *header, short vabId, u_long address) {
    (void)vabId;
    s_openHeader = header;
    s_openAddress = (s32)address;
    return s_openResult;
}

short SsVabTransBody(u8 *body, short vabId) {
    (void)vabId;
    s_body = body;
    return s_bodyResult;
}

short SsVabTransCompleted(short immediate) {
    (void)immediate;
    return s_completed;
}

void LoadAudioParameterTable(const void *data, size_t size) {
    s_tableData = data;
    s_tableSize = size;
    s_tableCalls++;
}

void SsUtSetReverbDepth(short left, short right) {
    (void)left; (void)right;
    s_reverbCalls++;
}

void _SsVmInit(int voices) {
    (void)voices;
    s_vmInitCalls++;
}

void SsVabClose(short vabId) { s_closeVab = vabId; }
void SpuVmDamperStep(void) { s_damperCalls++; }

void BiosExit(s32 code) {
    (void)code;
    abort();
}

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

enum {
    TEST_VAB_HEADER_SIZE = 32,
    TEST_VAB_PROGRAM_ATTRIBUTE_SIZE = 16,
    TEST_VAB_LENGTH_TABLE_ENTRIES = 256,
};

static void WriteLittleEndianU16(u8 *destination, u16 value) {
    destination[0] = (u8)value;
    destination[1] = (u8)(value >> 8);
}

static void WriteLittleEndianU32(u8 *destination, u32 value) {
    destination[0] = (u8)value;
    destination[1] = (u8)(value >> 8);
    destination[2] = (u8)(value >> 16);
    destination[3] = (u8)(value >> 24);
}

int main(void) {
    u8 header[40000];
    u8 body[16];
    u16 table[ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT];
    AudioSlotAsset asset;
    AudioSlotAsset invalid;
    u8 *vagLengths;

    memset(&g_SoundScale, 0, sizeof(g_SoundScale));
    memset(&g_EngineSoundState, 0, sizeof(g_EngineSoundState));
    memset(header, 0, sizeof(header));
    memset(body, 0, sizeof(body));
    memcpy(header, "pBAV", 4);
    WriteLittleEndianU32(header + 4, 4);
    WriteLittleEndianU16(header + 18, 0);
    WriteLittleEndianU16(header + 22, 0);
    vagLengths = header + TEST_VAB_HEADER_SIZE +
                 64 * TEST_VAB_PROGRAM_ATTRIBUTE_SIZE;
    WriteLittleEndianU16(vagLengths, 1);
    asset = (AudioSlotAsset){
        .vabHeader = header,
        .vabHeaderSize = TEST_VAB_HEADER_SIZE +
                         64 * TEST_VAB_PROGRAM_ATTRIBUTE_SIZE +
                         TEST_VAB_LENGTH_TABLE_ENTRIES * sizeof(u16),
        .vabBody = body,
        .vabBodySize = sizeof(body),
    };
    g_VabSpuAddress[0] = 0x12000;

    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == 1);
    CHECK(g_Audio.slots.loading == AUDIO_SLOT_MAIN_CUES &&
          g_SoundScale.vabIds[AUDIO_SLOT_MAIN_CUES] == 8);
    CHECK(s_openHeader == header && s_body == body);
    CHECK(s_openAddress == 0x12000);

    g_Audio.slots.loaded = 0;
    g_Audio.slots.cueBank = 0;
    CHECK(PollAudioSlotLoad() == 1);
    CHECK(g_Audio.slots.loaded == 1 && g_Audio.slots.cueBank == 1 &&
          g_Audio.slots.loading == -1);
    g_Audio.slots.loaded = 0;
    g_Audio.slots.cueBank = -1;
    CHECK(PollAudioSlotLoad() == 1);
    CHECK(g_Audio.slots.loaded == 0 && g_Audio.slots.cueBank == -1);

    invalid = asset;
    invalid.vabHeaderSize--;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &invalid) == -1);
    invalid = asset;
    invalid.vabBodySize = 3;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &invalid) == -1);
    header[1] = 'X';
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    header[1] = 'B';
    header[0] = 'X';
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    header[0] = 'p';
    WriteLittleEndianU16(header + 18, 65);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    WriteLittleEndianU16(header + 18, 0);
    header[0] = 'X';
    WriteLittleEndianU32(header + 4, 5);
    WriteLittleEndianU16(header + 18, 65);
    invalid = asset;
    invalid.vabHeaderSize = sizeof(header);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &invalid) == -1);
    header[0] = 'p';
    WriteLittleEndianU32(header + 4, 4);
    WriteLittleEndianU16(header + 18, 0);
    WriteLittleEndianU16(header + 22, 256);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    WriteLittleEndianU16(header + 22, 0);
    CHECK(StartAudioSlotLoad(6, &asset) == -1);
    CHECK(StartAudioSlotLoad(-1, &asset) == -1);

    g_Audio.slots.loading = 99;
    s_openResult = -1;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    CHECK(g_Audio.slots.loading == 99);
    s_openResult = 7;
    s_bodyResult = -1;
    s_closeVab = -1;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1 &&
          s_closeVab == 7);
    CHECK(g_Audio.slots.loading == 99);
    s_bodyResult = 8;

    g_VabSpuAddress[3] = 0x34000;
    asset.auxiliaryData = table;
    asset.auxiliarySize = sizeof(table);
    invalid = asset;
    invalid.auxiliarySize--;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_ENGINE, &invalid) == -1);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_ENGINE, &asset) == 1);
    CHECK(g_Audio.slots.loading == AUDIO_SLOT_ENGINE && s_openAddress == 0x34000);
    CHECK(s_tableCalls == 1 && s_tableData == table &&
          s_tableSize == sizeof(table));

    s_completed = 0;
    g_Audio.slots.loaded = 0;
    g_Audio.slots.cueBank = -1;
    CHECK(PollAudioSlotLoad() == 0);
    CHECK(g_Audio.slots.loaded == 0 && g_Audio.slots.cueBank == -1);

    s_completed = 1;
    g_Audio.slots.loading = -1;
    g_Audio.slots.loaded = 0;
    g_Audio.slots.cueBank = -1;
    CHECK(PollAudioSlotLoad() == 1);
    CHECK(g_Audio.slots.loaded == 0 && g_Audio.slots.cueBank == -1);

    g_Audio.slots.loading = 2;
    CHECK(PollAudioSlotLoad() == 1 && g_Audio.slots.cueBank == 2);
    g_Audio.slots.loading = 3;
    CHECK(PollAudioSlotLoad() == 1 && g_Audio.slots.cueBank == 2);

    g_Audio.slots.loaded = (1 << 0) | (1 << 2) | (1 << 3);
    g_Audio.slots.cueBank = 2;
    g_SoundScale.vabIds[2] = 22;
    g_SoundScale.vabIds[3] = 23;
    s_damperCalls = 0;
    s_reverbCalls = 0;
    s_vmInitCalls = 0;
    CloseLoadedAudioSlots();
    CHECK(s_damperCalls == 2 && g_Audio.slots.loaded == 1 && s_closeVab == 23);
    CHECK(s_reverbCalls == 2 && s_vmInitCalls == 2);
    CHECK(g_Audio.slots.cueBank == 1);

    g_Audio.slots.loaded = 1 << 3;
    g_Audio.slots.cueBank = 2;
    CloseLoadedAudioSlots();
    CHECK(g_Audio.slots.loaded == 0 && s_closeVab == 23 &&
          g_Audio.slots.cueBank == 0);

    puts("audio slot loading preserves VAB routing, polling, and close state");
    return 0;
}
