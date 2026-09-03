#include "game/audio.h"
#include "game/audio_internal.h"

void ForceAllEffectVoicesEnabled(s32 enabled) {
    ForcePanVoiceEnabled(enabled);
    ForceBasicEffectVoicesEnabled(enabled);
    ForceIndexedEffectVoiceEnabled(enabled);
    ForcePitchEffectVoicesEnabled(enabled);
    ForceSoundSlotVoicePlayback(enabled);
}
