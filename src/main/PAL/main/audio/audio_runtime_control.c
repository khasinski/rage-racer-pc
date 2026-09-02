#include "game/audio.h"
#include "game/sound.h"
#include "game/state.h"
#include "game/work_buffer.h"
#include "psyq/snd.h"
#include "timing_control.h"

/* libsnd counts musical time in ticks of a sixtieth of a second, which is what
 * retail asked for: SsSetTickMode(SS_TICK60) left VBLANK_MINUS at sixty and
 * drove SsSeqCalledTbyT from a counter interrupt at sixty hertz on a PAL
 * console as well, so the music kept its tempo while the picture ran at fifty.
 * The host has no such interrupt and services the sequencer from the game
 * loop, so it has to pay the same sixty ticks a second out of frames that
 * arrive at fifty. */
enum {
    SEQUENCE_TICK_HZ = 60,
    SOUND_MODE_SCENE_ID = 0xC,
    DEFAULT_REVERB_DEPTH = 0x28,
    FIRST_REVERB_PRESET = 1,
    LAST_REVERB_PRESET = 9,
    FIRST_SOUND_SLOT_VOICE = 0xE,
    SOUND_SLOT_NOTE = 0x3C,
};

void TickSequenceAudio(void) {
    /* Ticks the sequencer is owed, in units of one game frame. */
    static s32 tickCredit;

    if (g_SceneId == SOUND_MODE_SCENE_ID) {
        SpuVmDamperStep();
    } else {
        s32 frameHz = TimingBaseHz();

        tickCredit += SEQUENCE_TICK_HZ;
        while (tickCredit >= frameHz) {
            tickCredit -= frameHz;
            SsSeqCalledTbyT();
        }
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
        g_ReverbType = 0;
        g_ReverbDepthR = 0;
        g_ReverbDepthL = 0;
        return;
    }

    g_ReverbType = type;
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
