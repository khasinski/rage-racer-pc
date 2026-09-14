#include "game/audio.h"
#include "game/audio_internal.h"
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

static const s32 s_carVolumeScales[CAR_SOUND_VOLUME_SCALE_COUNT] = {
    20, 21, 22, 23, 21, 22, 23, 22, 23, 26, 27, 28, 29, 30, 50, 52,
    54, 50, 52, 54, 52, 42, 44, 28, 28, 29, 30, 31, 30, 26, 46, 80,
};

void InitSequenceAudio(void) {
    _SsVmInit(LIBSND_RESET);
    SsSetReservedVoice(SEQUENCE_VOICE_COUNT);
    SetDefaultReverbDepth();
    g_Audio.reverb.fade = 0;
    RefreshSequenceVolumeScale();
}

void InitEffectVoiceRuntime(void) {
    s32 carAssetIndex;

    SetSoundSlotVoicesEnabled(0);
    _SsVmInit(LIBSND_RESET);
    SsSetReservedVoice(EFFECT_VOICE_RUNTIME_COUNT);
    ResetAudioVoiceState();

    g_EngineSoundState.bank = -1;
    SetSoundSlotVoicesEnabled(1);
    SetReverbPreset(EFFECT_REVERB_PRESET, 0, 0);
    carAssetIndex = GetOwnedCarAssetIndex(g_PlayerCarIndex);
    if ((u32)carAssetIndex >= CAR_SOUND_VOLUME_SCALE_COUNT) {
        carAssetIndex = 0;
    }
    SetLoadedTableVolumeScale(s_carVolumeScales[carAssetIndex]);
}
