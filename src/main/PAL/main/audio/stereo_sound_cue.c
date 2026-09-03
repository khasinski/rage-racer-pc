#include "game/audio.h"
#include "game/sound.h"

enum {
    STEREO_SOUND_GROUP_SIZE = 2,
    SOUND_MODE_FACTOR_ONE = 128,
};

static s32 SoundModeChannelCount(const SoundModeEntry *mode) {
    if (mode->count < 0) {
        return 0;
    }
    return mode->count > AUDIO_MUSIC_CHANNEL_COUNT
               ? AUDIO_MUSIC_CHANNEL_COUNT
               : mode->count;
}

static int MusicChannelsOnMode(s32 mode) {
    return g_MusicChannels[0].left.value == g_SoundModes[mode].slots[0].left &&
           g_MusicChannels[1].left.value == g_SoundModes[mode].slots[1].left;
}

static void StopStereoSoundCue(s32 cue) {
    s32 groupStart = cue < STEREO_SOUND_GROUP_SIZE
                         ? 0
                         : STEREO_SOUND_GROUP_SIZE;
    s32 i;

    if (!MusicChannelsOnMode(groupStart) &&
        !MusicChannelsOnMode(groupStart + 1)) {
        return;
    }

    for (i = 0; i < SoundModeChannelCount(&g_SoundModes[cue]); i++) {
        g_MusicChannels[i].left.value = -1;
        g_MusicChannels[i].right.value = -1;
        g_MusicChannels[i].mode = MUSIC_CHANNEL_STOP;
        g_MusicChannels[i].volLeft = 0;
        g_MusicChannels[i].volRight = 0;
    }
}

void SetStereoSoundCue(s32 cue, s32 left, s32 right) {
    SoundModeEntry *soundMode;
    MusicChannelState state;
    s32 i;

    if (cue < 0) {
        cue = 0;
    } else if (cue >= AUDIO_SOUND_MODE_COUNT) {
        cue = AUDIO_SOUND_MODE_COUNT - 1;
    }

    left = ClampCueLevel(left);
    right = ClampCueLevel(right);
    if (left == 0 && right == 0) {
        StopStereoSoundCue(cue);
        return;
    }
    if (g_StereoOutput == 0) {
        left = (left + right) / 2;
        right = left;
    }

    soundMode = &g_SoundModes[cue];
    state = MusicChannelsOnMode(cue) ? MUSIC_CHANNEL_UPDATE
                                     : MUSIC_CHANNEL_START;

    for (i = 0; i < SoundModeChannelCount(soundMode); i++) {
        MusicChannel *channel = &g_MusicChannels[i];

        channel->left.value = soundMode->slots[i].left;
        channel->right.value = soundMode->slots[i].right;
        channel->mode = state;
        channel->volLeft = left * soundMode->factor / SOUND_MODE_FACTOR_ONE;
        channel->volRight = right * soundMode->factor / SOUND_MODE_FACTOR_ONE;
    }
}
