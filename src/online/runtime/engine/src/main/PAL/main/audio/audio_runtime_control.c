#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "psyq/snd.h"

/* The browser keeps only the engine-slot key-on from the desktop file: it has
 * no sequencer tick and no reverb unit (see web/wasm/web_spu.c). */

enum {
    FIRST_SOUND_SLOT_VOICE = 0xE,
    SOUND_SLOT_NOTE = 0x3C,
};

void PlaySoundSlotVoice(s32 slot, s32 tone, s32 vabSlot) {
    if ((u32)slot >= ENGINE_SOUND_SLOT_COUNT ||
        (u32)tone >= ENGINE_SOUND_BANK_COUNT ||
        (u32)vabSlot >= AUDIO_SLOT_COUNT) {
        return;
    }

    s16 hardwareVoice = (s16)(slot + FIRST_SOUND_SLOT_VOICE);
    s16 program = g_SoundSlotTone[slot][tone];

    SsUtKeyOnV(hardwareVoice, g_SoundScale.vabIds[vabSlot], program, 0,
               SOUND_SLOT_NOTE, 0, 0, 0);
}
