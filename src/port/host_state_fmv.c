/* Runtime state owned by full-motion-video playback and its scene return. */

#include "game/fmv.h"
#include "game/fmv_internal.h"

s32 g_FmvStreamEnded;
FmvPlaybackState g_FmvState;
s32 g_StreamReturnScene;
