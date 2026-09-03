#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

#include <stdio.h>
#include <stdlib.h>

SoundScale g_SoundScale;
SequenceHandle g_SeqHandle;
s32 g_SeqVolumeFadeStep;
s32 g_AudioLoadSlot;
s32 g_AudioLoadedSlotMask;
s32 g_VabSpuAddress[AUDIO_SLOT_COUNT];
const char g_MsgSeqVabOpenHeadError[] = "open";
const char g_MsgSeqVabTransBodyError[] = "body";

static s16 s_openResult = 7;
static s16 s_bodyResult = 8;
static s16 s_transferResult = 1;
static s16 s_sequenceOpenResult = (s16)0x8056;
static u8 *s_openHeader;
static u8 *s_transferBody;
static unsigned long s_openAddress;
static unsigned long *s_sequenceData;
static short s_sequenceVab;
static s32 s_reverbCalls;
static s32 s_vmInitCalls;
static s32 s_closedSequence;
static s32 s_closedVab;
static s32 s_failures;

short SsVabOpenHeadSticky(u8 *header, short vabId, unsigned long address) {
    if (vabId != -1) abort();
    s_openHeader = header;
    s_openAddress = address;
    return s_openResult;
}
short SsVabTransBody(u8 *body, short vabId) {
    if (vabId != s_openResult) abort();
    s_transferBody = body;
    return s_bodyResult;
}
short SsSeqOpen(unsigned long *sequence, short vabId) {
    s_sequenceData = sequence;
    s_sequenceVab = vabId;
    return s_sequenceOpenResult;
}
short SsVabTransCompleted(short immediate) {
    if (immediate != 0) abort();
    return s_transferResult;
}
void SsUtSetReverbDepth(short left, short right) {
    if (left == 0 && right == 0) s_reverbCalls++;
}
void _SsVmInit(int voices) {
    if (voices == 0) s_vmInitCalls++;
}
void SsSeqClose(short sequence) { s_closedSequence = sequence; }
void SsVabClose(short vabId) { s_closedVab = vabId; }
void BiosExit(s32 code) {
    (void)code;
    abort();
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

int main(void) {
    u8 header[4];
    u8 body[4];
    u8 sequence[4];

    g_VabSpuAddress[1] = 0x24000;
    g_SeqVolumeFadeStep = -4;
    Check(OpenSequenceAudioSlot(header, body, sequence) == 1,
          "sequence slot returns asynchronous transfer state");
    Check(g_AudioLoadSlot == 1 && g_SoundScale.vabIds[1] == 8 &&
              s_openHeader == header && s_transferBody == body &&
              s_openAddress == 0x24000,
          "sequence slot opens and transfers its VAB");
    Check(s_sequenceData == (unsigned long *)(void *)sequence &&
              s_sequenceVab == 8 &&
              g_SeqHandle.storage == (s16)0x8056 &&
              g_SeqVolumeFadeStep == 0,
          "sequence slot opens score and clears fade state");

    g_AudioLoadSlot = 99;
    s_openResult = -1;
    Check(OpenSequenceAudioSlot(header, body, sequence) == -1,
          "sequence VAB header failure is reported");
    Check(g_AudioLoadSlot == 99,
          "failed sequence open does not publish its slot");
    s_openResult = 7;
    s_bodyResult = -1;
    s_closedVab = -1;
    Check(OpenSequenceAudioSlot(header, body, sequence) == -1 &&
              s_closedVab == 7,
          "sequence VAB body failure closes its header");
    s_bodyResult = 8;
    s_sequenceOpenResult = -1;
    s_closedVab = -1;
    Check(OpenSequenceAudioSlot(header, body, sequence) == -1 &&
              s_closedVab == 8,
          "sequence-open failure closes its VAB");
    Check(g_AudioLoadSlot == 99,
          "failed sequence score does not publish its slot");
    s_sequenceOpenResult = (s16)0x8056;

    g_AudioLoadedSlotMask = 0;
    CloseSequenceAudioSlot();
    Check(s_reverbCalls == 0,
          "absent sequence slot is not closed");

    g_AudioLoadedSlotMask = (1 << 1) | (1 << 3);
    CloseSequenceAudioSlot();
    Check(g_AudioLoadedSlotMask == (1 << 3) && s_reverbCalls == 1 &&
              s_vmInitCalls == 1 &&
              s_closedSequence == (s16)0x8056 && s_closedVab == 8,
          "sequence close clears only its bit and releases SEQ and VAB");

    if (s_failures != 0) return 1;
    puts("VAB sequence slots open, track, and close their resources");
    return 0;
}
