/* Load CRT declarations before redirecting diagnostics: Windows implements
 * printf inline, so overriding the CRT symbol itself is not portable. */
#include <stdio.h>
#include "game/diagnostics.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "psyq/snd.h"
#include "game/sound.h"

int RageTestPrintf(const char *format, ...);
#define printf RageTestPrintf
#include "../../src/main/PAL/main/audio/sound_cues.c"
