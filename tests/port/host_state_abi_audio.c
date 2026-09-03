#include "../../src/port/host_state_audio.c"

_Static_assert(sizeof(g_IndexedEffects) ==
                   AUDIO_INDEXED_EFFECT_COUNT * sizeof(IndexedEffect),
               "indexed-effect table shape changed");
_Static_assert(sizeof(g_SoundModes) ==
                   AUDIO_SOUND_MODE_COUNT * sizeof(SoundModeEntry),
               "sound-mode table shape changed");
_Static_assert(sizeof(g_SoundSlotTone) ==
                   ENGINE_SOUND_SLOT_COUNT * ENGINE_SOUND_BANK_COUNT *
                       sizeof(s16),
               "sound-slot tone table shape changed");
_Static_assert(sizeof(g_EngineSoundCurves) == 1728,
               "engine-sound curve table shape changed");
_Static_assert(sizeof(g_EngineSoundState) == sizeof(EngineSoundState),
               "engine-sound state type changed");
_Static_assert(sizeof(g_MusicChannels) == 2 * sizeof(MusicChannel),
               "music-channel table shape changed");
_Static_assert(sizeof(g_EffectVoices) == 4 * sizeof(EffectVoice),
               "effect-voice table shape changed");
_Static_assert(sizeof(g_SeqHandle) == sizeof(SequenceHandle),
               "sequence handle type changed");
