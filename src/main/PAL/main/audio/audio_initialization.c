#include "game/audio.h"
#include "game/audio_state_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"
#include "game/car.h"

enum {
    LIBSND_RESET = 0,
    SEQUENCE_VOICE_COUNT = 18,
    EFFECT_VOICE_RUNTIME_COUNT = 8,
    EFFECT_REVERB_PRESET = 2,
};

void InitSequenceAudio(void) {
    _SsVmInit(LIBSND_RESET);
    SsSetVoiceCount(SEQUENCE_VOICE_COUNT);
    SetDefaultReverbDepth();
    g_ReverbFadeStep = 0;
    RefreshSequenceVolumeScale();
}

void InitEffectVoiceRuntime(void) {
    _SsVmInit(LIBSND_RESET);
    SsSetVoiceCount(EFFECT_VOICE_RUNTIME_COUNT);
    ResetAudioVoiceState();

    SetSoundSlotVoicesEnabled(1);
    SetReverbPreset(EFFECT_REVERB_PRESET, 0, 0);
    SetLoadedTableVolumeScale(
        g_CarSoundVolumeScales[GetOwnedCarAssetIndex(g_PlayerCarIndex)]);
}
