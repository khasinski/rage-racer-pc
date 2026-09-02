#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>

s32 g_ReverbDepthL;
s32 g_ReverbDepthR;
s32 g_ReverbFadeStep;
SequenceHandle g_SeqHandle;
s32 g_SeqVolume;
s32 g_SeqVolumeFadeStep;

static s32 s_closeSlot;
static s32 s_reverbLeft;
static s32 s_reverbRight;
static s32 s_sequenceLeft;
static s32 s_sequenceRight;
static s32 s_sequenceStops;
static s32 s_setVolume;
static s32 s_sequencePlays;
static s32 s_playMode;
static s32 s_loopCount;

void SsSeqPlay(short sequence, char playMode, short loopCount) {
    (void)sequence;
    s_sequencePlays++;
    s_playMode = playMode;
    s_loopCount = loopCount;
}
void SsSeqStop(short sequence) {
    (void)sequence;
    s_sequenceStops++;
}
void SsSeqSetVol(short sequence, short left, short right) {
    (void)sequence;
    s_sequenceLeft = left;
    s_sequenceRight = right;
}
void SetReverbDepth(s32 left, s32 right) {
    s_reverbLeft = left;
    s_reverbRight = right;
}
void SetDefaultReverbDepth(void) { SetReverbDepth(0x28, 0x28); }
void SetSequenceVolume(s32 volume) { s_setVolume = volume; }
int CloseAudioSlot(s32 slot) {
    s_closeSlot = slot;
    return 0;
}

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

int main(void) {
    g_SeqHandle.value = 7;
    PlaySequence();
    CHECK(s_sequencePlays == 1 && s_playMode == 1 && s_loopCount == 0);
    StopSequence();
    CHECK(s_sequenceStops == 1);

    StartSequenceFadeOut();
    CHECK(g_SeqVolumeFadeStep == -4 && g_ReverbFadeStep == -3);

    g_SeqVolume = 100;
    ApplyDuckedSequenceAudio();
    CHECK(s_sequenceLeft == 75 && s_sequenceRight == 75);
    CHECK(s_reverbLeft == 0x3C && s_reverbRight == 0x3C);

    g_SeqVolume = -5;
    ApplyDuckedSequenceAudio();
    CHECK(s_sequenceLeft == -3 && s_sequenceRight == -3);

    g_ReverbDepthL = 2;
    g_ReverbDepthR = 4;
    g_ReverbFadeStep = -3;
    g_SeqVolume = 3;
    g_SeqVolumeFadeStep = -4;
    s_closeSlot = -1;
    UpdateSequenceFadeOut();
    CHECK(g_ReverbDepthL == 0 && g_ReverbDepthR == 1);
    CHECK(g_ReverbFadeStep == -3);
    CHECK(g_SeqVolume == 0 && g_SeqVolumeFadeStep == 0);
    CHECK(s_sequenceStops == 2 && s_closeSlot == 6);
    CHECK(s_reverbLeft == 0x28 && s_reverbRight == 0x28);
    CHECK(s_setVolume == 0);

    g_SeqVolume = 10;
    g_SeqVolumeFadeStep = 0;
    UpdateSequenceFadeOut();
    CHECK(g_ReverbDepthR == 0 && g_ReverbFadeStep == 0);

    puts("sequence playback preserves ducking and fade completion");
    return 0;
}
