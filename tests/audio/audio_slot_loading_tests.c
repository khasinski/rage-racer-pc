#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SoundScale g_SoundScale;
EngineSoundState g_EngineSoundState;
s32 g_AudioLoadSlot;
s32 g_AudioLoadedSlotMask;
s32 g_SoundCueBank;
s32 g_VabSpuAddress[4];
s32 g_VabTransferDone;
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

s32 OpenVabSequenceSlot(s32 slot, u8 *header, u8 *body, void *sequence) {
    (void)slot; (void)header; (void)body; (void)sequence;
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

void _SsVmInit(short voices) {
    (void)voices;
    s_vmInitCalls++;
}

void SsVabClose(short vabId) { s_closeVab = vabId; }
void SpuVmDamperStep(void) { s_damperCalls++; }
s32 CloseAudioSlot(s32 slot) {
    (void)slot;
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

int main(void) {
    u8 header;
    u8 body;
    u16 table;

    memset(&g_SoundScale, 0, sizeof(g_SoundScale));
    memset(&g_EngineSoundState, 0, sizeof(g_EngineSoundState));
    g_VabSpuAddress[0] = 0x12000;

    CHECK(StartAudioSlotLoad(0, &header, &body, 0) == 1);
    CHECK(g_AudioLoadSlot == 0 && g_SoundScale.vabIds[0] == 8);
    CHECK(s_openHeader == &header && s_body == &body);
    CHECK(s_openAddress == 0x12000);

    g_AudioLoadedSlotMask = 0;
    g_SoundCueBank = 0;
    CHECK(PollAudioSlotLoad() == 1);
    CHECK(g_AudioLoadedSlotMask == 1 && g_SoundCueBank == 1);

    CHECK(StartAudioSlotLoad(1, &header, &body, &table) == 61);
    CHECK(s_sequenceCalls == 1);
    CHECK(StartAudioSlotLoad(6, &header, &body, &table) == 61);
    CHECK(s_sequenceCalls == 2);

    g_VabSpuAddress[3] = 0x34000;
    CHECK(StartAudioSlotLoad(3, &header, &body, &table) == 1);
    CHECK(g_AudioLoadSlot == 3 && s_openAddress == 0x34000);
    CHECK(g_EngineSoundState.extraVabLoaded == 1 && s_tableCalls == 1);

    s_completed = 0;
    g_AudioLoadedSlotMask = 0;
    g_SoundCueBank = -1;
    CHECK(PollAudioSlotLoad() == 0);
    CHECK(g_AudioLoadedSlotMask == 0 && g_SoundCueBank == -1);

    s_completed = 1;
    g_AudioLoadSlot = 1;
    CHECK(PollAudioSlotLoad() == 1 && g_SoundCueBank == 1);
    g_AudioLoadSlot = 2;
    CHECK(PollAudioSlotLoad() == 1 && g_SoundCueBank == 2);
    g_AudioLoadSlot = 3;
    CHECK(PollAudioSlotLoad() == 1 && g_SoundCueBank == 2);

    g_AudioLoadedSlotMask = 1 << 2;
    g_SoundScale.vabIds[2] = 12;
    CHECK(CloseVabOnlyAudioSlot(2) == 1);
    CHECK(g_AudioLoadedSlotMask == 0 && s_closeVab == 12);
    CHECK(s_reverbCalls == 1 && s_vmInitCalls == 1);
    CHECK(CloseVabOnlyAudioSlot(2) == 0);

    g_AudioLoadedSlotMask = (1 << 2) | (1 << 3);
    g_SoundScale.vabIds[2] = 22;
    g_SoundScale.vabIds[3] = 23;
    s_closeAudioCalls = 0;
    s_damperCalls = 0;
    s_closeAudioResult = 1;
    CHECK(CloseLoadedAudioSlots() == 1);
    CHECK(s_damperCalls == 1 && s_closeAudioCalls == 1 &&
          g_AudioLoadedSlotMask == 0 && s_closeVab == 23);

    g_AudioLoadedSlotMask = 1 << 3;
    CHECK(CloseLoadedAudioSlots() == 0);
    CHECK(g_AudioLoadedSlotMask == (1 << 3));

    puts("audio slot loading preserves VAB routing, polling, and close state");
    return 0;
}
