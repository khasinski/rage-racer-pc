#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/sound.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/work_buffer.h"
#include "psyq/snd.h"

enum {
    DEFAULT_REVERB_DEPTH = 0x28,
    FIRST_REVERB_PRESET = 1,
    LAST_REVERB_PRESET = 9,
    FIRST_SOUND_SLOT_VOICE = 0xE,
    SOUND_SLOT_NOTE = 0x3C,
};

void TickSequenceAudio(void) {
    if (g_SceneId == GAME_SCENE_RACE) {
        SpuVmDamperStep();
    } else {
        /* Menu SEQ data advances with the game frame. Advancing it at a
         * synthetic 60 Hz on PAL accelerated the note stream by 20% while
         * leaving each VAG sample's pitch unchanged. */
        SsSeqCalledTbyT();
        if (g_SeqVolumeFadeStep != 0) {
            UpdateSequenceFadeOut();
        }
        /* libsnd batches voice register writes. On PS1 its sound interrupt
         * flushed the final key-off; the host drives this service directly. */
        SpuVmDamperStep();
    }
}

void SetReverbDepth(s32 left, s32 right) {
    left = ClampCueLevel(left);
    right = ClampCueLevel(right);

    g_ReverbDepthL = left;
    g_ReverbDepthR = right;
    SsUtSetReverbDepth((s16)left, (s16)right);
}

void SetDefaultReverbDepth(void) {
    SetReverbDepth(DEFAULT_REVERB_DEPTH, DEFAULT_REVERB_DEPTH);
}

static int IsValidReverbPreset(s32 type) {
    return type >= FIRST_REVERB_PRESET && type <= LAST_REVERB_PRESET;
}

void SetReverbPreset(s32 type, s32 left, s32 right) {
    SsUtReverbOff();

    if (!IsValidReverbPreset(type)) {
        g_ReverbDepthR = 0;
        g_ReverbDepthL = 0;
        return;
    }

    SsUtSetReverbType((s16)type);
    SsUtReverbOn();
    SetReverbDepth(left, right);
}

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
