#include "psyq/snd.h"
/* Stop the effect voice for sound slot `slot` (hardware voice = slot + 0xE). */
void StopSoundSlotVoice(s16 slot) { SsUtKeyOffV((s16)(slot + 14)); }
