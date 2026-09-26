#include "game/audio.h"
#include "game/car_internal.h"
#include "game/engine_sound.h"
#include "game/integer.h"
#include "game/random.h"
#include "game/state.h"

void UpdatePlayerEnginePresentation(const PlayerCarRuntime *car, const GameCarSpec *spec, int finished) {
    EngineSound sound = {.rpm = g_EngineRpm};
    if (!car || !StepEngineSound(&sound, &car->drive, spec,
                                (u32)g_AnimTimer, g_RandomSeed)) return;
    g_EngineRpm = sound.rpm;
    g_EngineRpmJitter = sound.jitter;
    g_TachoShiftLightOn = sound.shiftLight;
    if (finished) SetIndexedEffectVoice(-1, 0, 0);
    UpdateLoadedAudioVoices(WrapSigned32((int64_t)sound.rpm + sound.jitter), sound.powered);
}
