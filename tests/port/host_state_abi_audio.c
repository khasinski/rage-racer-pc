#include "../../src/port/host_state_audio.c"

_Static_assert(sizeof(g_IndexedEffects) == 3 * sizeof(IndexedEffect),
               "indexed-effect table shape changed");
_Static_assert(sizeof(g_SoundModes) == 4 * sizeof(SoundModeEntry),
               "sound-mode table shape changed");
_Static_assert(sizeof(g_SoundSlotTone) == 6 * 2 * sizeof(s16),
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
