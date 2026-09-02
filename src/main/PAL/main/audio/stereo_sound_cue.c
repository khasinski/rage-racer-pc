#include "game/audio.h"
#include "game/sound.h"

static int MusicChannelsOnMode(s32 mode) {
    return g_MusicChannels[0].left.value == g_SoundModes[mode].slots[0].left &&
           g_MusicChannels[1].left.value == g_SoundModes[mode].slots[1].left;
}

static void StopStereoSoundCue(s32 cue) {
    s32 groupStart = cue < 2 ? 0 : 2;
    s32 i;

    if (!MusicChannelsOnMode(groupStart) &&
        !MusicChannelsOnMode(groupStart + 1)) {
        return;
    }

    for (i = 0; i < g_SoundModes[cue].count; i++) {
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
    } else if (cue > 3) {
        cue = 3;
    }

    left = ClampCueLevel(left);
    right = ClampCueLevel(right);
    if (left == 0 && right == 0) {
        StopStereoSoundCue(cue);
        return;
    }

    soundMode = &g_SoundModes[cue];
    state = MusicChannelsOnMode(cue) ? MUSIC_CHANNEL_UPDATE
                                     : MUSIC_CHANNEL_START;

    for (i = 0; i < soundMode->count; i++) {
        MusicChannel *channel = &g_MusicChannels[i];
        s32 channelLeft = left;
        s32 channelRight = right;

        if (g_StereoOutput == 0) {
            channelLeft = (left + right) / 2;
            channelRight = channelLeft;
        }

        channel->left.value = soundMode->slots[i].left;
        channel->right.value = soundMode->slots[i].right;
        channel->mode = state;
        channel->volLeft = channelLeft * soundMode->factor / 128;
        channel->volRight = channelRight * soundMode->factor / 128;
    }
}
