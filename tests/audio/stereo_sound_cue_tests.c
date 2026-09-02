#include "common.h"
#include "game/audio.h"
#include "game/sound.h"

#include <stdio.h>
#include <string.h>

MusicChannel g_MusicChannels[3];
SoundModeEntry g_SoundModes[4];
s32 g_StereoOutput;

#define CHECK(condition) do {                                                   \
    if (!(condition)) {                                                         \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void Reset(void) {
    s32 mode;
    s32 channel;

    memset(g_MusicChannels, 0, sizeof(g_MusicChannels));
    memset(g_SoundModes, 0, sizeof(g_SoundModes));
    for (mode = 0; mode < 4; mode++) {
        g_SoundModes[mode].count = 2;
        g_SoundModes[mode].factor = 64 + mode * 16;
        for (channel = 0; channel < 2; channel++) {
            g_SoundModes[mode].slots[channel].left = 10 + mode * 2 + channel;
            g_SoundModes[mode].slots[channel].right = mode;
        }
    }
    g_MusicChannels[0].left.value = -1;
    g_MusicChannels[1].left.value = -1;
    g_StereoOutput = 1;
}

int main(void) {
    Reset();
    SetStereoSoundCue(1, 64, 32);
    CHECK(g_MusicChannels[0].left.value == 12);
    CHECK(g_MusicChannels[1].left.value == 13);
    CHECK(g_MusicChannels[0].mode == MUSIC_CHANNEL_START &&
          g_MusicChannels[1].mode == MUSIC_CHANNEL_START);
    CHECK(g_MusicChannels[0].volLeft == 40);
    CHECK(g_MusicChannels[0].volRight == 20);

    SetStereoSoundCue(1, 32, 16);
    CHECK(g_MusicChannels[0].mode == MUSIC_CHANNEL_UPDATE &&
          g_MusicChannels[1].mode == MUSIC_CHANNEL_UPDATE);

    Reset();
    g_StereoOutput = 0;
    SetStereoSoundCue(2, 100, 20);
    CHECK(g_MusicChannels[0].volLeft == 45);
    CHECK(g_MusicChannels[0].volRight == 45);

    Reset();
    SetStereoSoundCue(-5, 200, 200);
    CHECK(g_MusicChannels[0].left.value == 10);
    CHECK(g_MusicChannels[0].volLeft == 63);
    CHECK(g_MusicChannels[0].volRight == 63);
    SetStereoSoundCue(9, 64, 64);
    CHECK(g_MusicChannels[0].left.value == 16);

    Reset();
    SetStereoSoundCue(0, 64, 64);
    SetStereoSoundCue(1, 0, 0);
    CHECK(g_MusicChannels[0].left.value == -1);
    CHECK(g_MusicChannels[1].left.value == -1);
    CHECK(g_MusicChannels[0].mode == MUSIC_CHANNEL_STOP &&
          g_MusicChannels[1].mode == MUSIC_CHANNEL_STOP);
    CHECK(g_MusicChannels[0].volLeft == 0 && g_MusicChannels[0].volRight == 0);

    Reset();
    SetStereoSoundCue(0, 64, 64);
    SetStereoSoundCue(2, 0, 0);
    CHECK(g_MusicChannels[0].left.value == 10);
    CHECK(g_MusicChannels[0].mode == MUSIC_CHANNEL_START);

    Reset();
    g_SoundModes[0].count = 99;
    g_MusicChannels[2].left.value = 1234;
    SetStereoSoundCue(0, 64, 64);
    CHECK(g_MusicChannels[0].left.value == 10 &&
          g_MusicChannels[1].left.value == 11);
    CHECK(g_MusicChannels[2].left.value == 1234);

    puts("stereo sound cues preserve routing, reuse, mono mix, and stop groups");
    return 0;
}
