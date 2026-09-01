#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"
#include "game/car.h"

void InitSequenceAudio(void) {
    _SsVmInit(0);
    SsSetVoiceCount(0x12);
    SetReverbDepth(0x28, 0x28);
    g_ReverbFadeStep = 0;
    RefreshSequenceVolumeScale();
}

void InitEffectVoiceRuntime(void) {
    s32 i;

    _SsVmInit(0);
    SsSetVoiceCount(8);

    for (i = 0; i < 2; i++) {
        g_MusicChannels[i].mode = -1;
        g_MusicChannels[i].left.value = -1;
        g_MusicChannels[i].right.value = -1;
        g_MusicChannels[i].volLeft = 0;
        g_MusicChannels[i].volRight = 0;
    }

    for (i = 0; i < 4; i++) {
        g_EffectVoices[i].state = -1;
        g_EffectVoices[i].note.value = -1;
        g_EffectVoices[i].tone = -1;
        g_EffectVoices[i].pitch.value = 0x1E00;
        g_EffectVoices[i].volume = 0;
    }

    g_PanVoiceVolumeR = -1;
    g_PanVoiceVolumeL = -1;
    g_IndexedEffectIndexPrev = -1;
    g_IndexedEffectIndex = -1;
    g_PanVoiceActive = 0;
    g_IndexedEffectPitch = 0x1E00;

    SetEffectVoicesEnabled(1);
    SetReverbPreset(2, 0, 0);
    SetLoadedTableVolumeScale(g_CarSoundVolumeScales[GetOwnedCarAssetIndex(g_PlayerCarIndex)]);
}
