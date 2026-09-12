#include "common.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"

#include <limits.h>
#include <stdio.h>

Audio g_Audio;

static s32 s_closeCalls;
static s32 s_reverbLeft;
static s32 s_reverbRight;
static s32 s_sequenceLeft;
static s32 s_sequenceRight;
static s32 s_sequenceStops;
static s32 s_setVolume;
static s32 s_setVolumeCalls;
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
void SetSequenceVolume(s32 volume) {
    s_setVolume = volume;
    s_setVolumeCalls++;
}
void CloseSequenceAudioSlot(void) {
    s_closeCalls++;
}

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

int main(void) {
    g_Audio.seq.handle = 7;
    PlaySequence();
    CHECK(s_sequencePlays == 1 && s_playMode == 1 && s_loopCount == 0);

    StartSequenceFadeOut();
    CHECK(g_Audio.seq.fade == -4 && g_Audio.reverb.fade == -3);

    g_Audio.seq.volume = 100;
    ApplyDuckedSequenceAudio();
    CHECK(s_sequenceLeft == 75 && s_sequenceRight == 75);
    CHECK(s_reverbLeft == 0x3C && s_reverbRight == 0x3C);

    g_Audio.seq.volume = -5;
    ApplyDuckedSequenceAudio();
    CHECK(s_sequenceLeft == 0 && s_sequenceRight == 0);

    g_Audio.seq.volume = INT_MAX;
    ApplyDuckedSequenceAudio();
    CHECK(s_sequenceLeft == 96 && s_sequenceRight == 96);

    g_Audio.seq.volume = INT_MAX;
    ApplyCurrentSequenceAudio();
    CHECK(s_sequenceLeft == 0x80 && s_sequenceRight == 0x80);
    CHECK(s_reverbLeft == 0x28 && s_reverbRight == 0x28);

    g_Audio.reverb.left = 2;
    g_Audio.reverb.right = 4;
    g_Audio.reverb.fade = -3;
    g_Audio.seq.volume = 3;
    g_Audio.seq.fade = -4;
    s_closeCalls = 0;
    s_setVolumeCalls = 0;
    UpdateSequenceFadeOut();
    CHECK(g_Audio.reverb.left == 0 && g_Audio.reverb.right == 1);
    CHECK(g_Audio.reverb.fade == -3);
    CHECK(g_Audio.seq.volume == 0 && g_Audio.seq.fade == 0);
    CHECK(s_sequenceStops == 1 && s_closeCalls == 1);
    CHECK(s_reverbLeft == 0x28 && s_reverbRight == 0x28);
    CHECK(s_setVolumeCalls == 0);

    g_Audio.seq.volume = 10;
    g_Audio.seq.fade = 0;
    UpdateSequenceFadeOut();
    CHECK(g_Audio.reverb.right == 0 && g_Audio.reverb.fade == 0);
    CHECK(s_setVolumeCalls == 1 && s_setVolume == 10);

    g_Audio.reverb.left = INT_MAX;
    g_Audio.reverb.right = 1;
    g_Audio.reverb.fade = INT_MIN;
    g_Audio.seq.volume = INT_MAX;
    g_Audio.seq.fade = INT_MIN;
    UpdateSequenceFadeOut();
    CHECK(g_Audio.reverb.left == 0 && g_Audio.reverb.right == 0);
    CHECK(g_Audio.seq.volume == 0 && g_Audio.seq.fade == 0);
    CHECK(s_sequenceStops == 2 && s_closeCalls == 2);

    g_Audio.reverb.left = 10;
    g_Audio.reverb.right = 20;
    g_Audio.reverb.fade = 3;
    g_Audio.seq.volume = 30;
    g_Audio.seq.fade = 4;
    UpdateSequenceFadeOut();
    CHECK(g_Audio.reverb.left == 10 && g_Audio.reverb.right == 20 &&
          g_Audio.reverb.fade == 0);
    CHECK(g_Audio.seq.volume == 30 && g_Audio.seq.fade == 0);
    CHECK(s_setVolume == 30);

    puts("sequence playback preserves ducking and fade completion");
    return 0;
}
