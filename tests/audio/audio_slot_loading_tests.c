#include "common.h"
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SoundScale g_SoundScale;
EngineSoundState g_EngineSoundState;
s32 g_AudioLoadSlot;
s32 g_AudioLoadedSlotMask;
s32 g_SoundCueBank;
s32 g_VabSpuAddress[4];
char g_MsgVabOpenHeadError[] = "open";
char g_MsgVabTransBodyError[] = "body";

static s16 s_openResult = 7;
static s16 s_bodyResult = 8;
static s16 s_completed = 1;
static s32 s_openAddress;
static u8 *s_openHeader;
static u8 *s_body;
static s32 s_sequenceCalls;
static s32 s_tableCalls;
static s32 s_closeVab;
static s32 s_reverbCalls;
static s32 s_vmInitCalls;
static s32 s_damperCalls;
static s32 s_closeAudioResult = 1;
static s32 s_closeAudioCalls;

short SsVabOpenHeadSticky(u8 *header, short vabId, unsigned long address) {
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

s32 OpenSequenceAudioSlot(u8 *header, u8 *body, void *sequence) {
    (void)header; (void)body; (void)sequence;
    s_sequenceCalls++;
    return 61;
}

void LoadAudioParameterTable(const u16 *table) {
    (void)table;
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
s32 CloseSequenceAudioSlot(void) {
    s_closeAudioCalls++;
    return s_closeAudioResult;
}

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
    u8 header[2048];
    u8 body[16];
    u8 sequence[16];
    u16 table[ENGINE_SOUND_PARAMETER_TABLE_WORD_COUNT];
    AudioSlotAsset asset;
    AudioSlotAsset invalid;
    u8 *vagLengths;

    memset(&g_SoundScale, 0, sizeof(g_SoundScale));
    memset(&g_EngineSoundState, 0, sizeof(g_EngineSoundState));
    memset(header, 0, sizeof(header));
    memset(body, 0, sizeof(body));
    memset(sequence, 0, sizeof(sequence));
    memcpy(header, "pBAV", 4);
    WriteLittleEndianU32(header + 4, 4);
    WriteLittleEndianU16(header + 18, 0);
    WriteLittleEndianU16(header + 22, 0);
    vagLengths = header + TEST_VAB_HEADER_SIZE +
                 64 * TEST_VAB_PROGRAM_ATTRIBUTE_SIZE;
    WriteLittleEndianU16(vagLengths, 1);
    sequence[0] = 'p';
    sequence[7] = 1;
    sequence[9] = 1;
    sequence[12] = 1;
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
    CHECK(g_AudioLoadSlot == AUDIO_SLOT_MAIN_CUES &&
          g_SoundScale.vabIds[AUDIO_SLOT_MAIN_CUES] == 8);
    CHECK(s_openHeader == header && s_body == body);
    CHECK(s_openAddress == 0x12000);

    g_AudioLoadedSlotMask = 0;
    g_SoundCueBank = 0;
    CHECK(PollAudioSlotLoad() == 1);
    CHECK(g_AudioLoadedSlotMask == 1 && g_SoundCueBank == 1);

    asset.auxiliaryData = sequence;
    asset.auxiliarySize = sizeof(sequence);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_SEQUENCE, &asset) == 61);
    CHECK(s_sequenceCalls == 1);
    invalid = asset;
    invalid.auxiliaryData = NULL;
    invalid.auxiliarySize = 0;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_SEQUENCE, &invalid) == -1);
    invalid = asset;
    invalid.auxiliarySize = sizeof(sequence) - 1;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_SEQUENCE, &invalid) == -1);
    invalid = asset;
    invalid.vabHeaderSize--;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &invalid) == -1);
    invalid = asset;
    invalid.vabBodySize = 3;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &invalid) == -1);
    header[1] = 'X';
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    header[1] = 'B';
    WriteLittleEndianU16(header + 18, 65);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    WriteLittleEndianU16(header + 18, 0);
    WriteLittleEndianU16(header + 22, 256);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    WriteLittleEndianU16(header + 22, 0);
    CHECK(StartAudioSlotLoad(6, &asset) == -1);
    CHECK(StartAudioSlotLoad(-1, &asset) == -1);
    CHECK(s_sequenceCalls == 1);

    s_openResult = -1;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    s_openResult = 7;
    s_bodyResult = -1;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_MAIN_CUES, &asset) == -1);
    s_bodyResult = 8;

    g_VabSpuAddress[3] = 0x34000;
    asset.auxiliaryData = table;
    asset.auxiliarySize = sizeof(table);
    invalid = asset;
    invalid.auxiliarySize--;
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_ENGINE, &invalid) == -1);
    CHECK(StartAudioSlotLoad(AUDIO_SLOT_ENGINE, &asset) == 1);
    CHECK(g_AudioLoadSlot == AUDIO_SLOT_ENGINE && s_openAddress == 0x34000);
    CHECK(g_EngineSoundState.extraVabLoaded == 1 && s_tableCalls == 1);

    s_completed = 0;
    g_AudioLoadedSlotMask = 0;
    g_SoundCueBank = -1;
    CHECK(PollAudioSlotLoad() == 0);
    CHECK(g_AudioLoadedSlotMask == 0 && g_SoundCueBank == -1);

    s_completed = 1;
    g_AudioLoadSlot = -1;
    g_AudioLoadedSlotMask = 0;
    g_SoundCueBank = -1;
    CHECK(PollAudioSlotLoad() == 1);
    CHECK(g_AudioLoadedSlotMask == 0 && g_SoundCueBank == -1);

    g_AudioLoadSlot = 1;
    CHECK(PollAudioSlotLoad() == 1 && g_SoundCueBank == 1);
    g_AudioLoadSlot = 2;
    CHECK(PollAudioSlotLoad() == 1 && g_SoundCueBank == 2);
    g_AudioLoadSlot = 3;
    CHECK(PollAudioSlotLoad() == 1 && g_SoundCueBank == 2);

    g_AudioLoadedSlotMask = (1 << 2) | (1 << 3);
    g_SoundScale.vabIds[2] = 22;
    g_SoundScale.vabIds[3] = 23;
    s_closeAudioCalls = 0;
    s_damperCalls = 0;
    s_reverbCalls = 0;
    s_vmInitCalls = 0;
    s_closeAudioResult = 1;
    CloseLoadedAudioSlots();
    CHECK(s_damperCalls == 1 && s_closeAudioCalls == 1 &&
          g_AudioLoadedSlotMask == 0 && s_closeVab == 23);
    CHECK(s_reverbCalls == 2 && s_vmInitCalls == 2);

    g_AudioLoadedSlotMask = 1 << 3;
    s_closeAudioResult = 0;
    CloseLoadedAudioSlots();
    CHECK(g_AudioLoadedSlotMask == 0 && s_closeVab == 23);

    puts("audio slot loading preserves VAB routing, polling, and close state");
    return 0;
}
