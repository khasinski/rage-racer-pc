#include "psyq/snd.h"
#include "psyq/snd_internal.h"

/*
 * ContDataEntry - the real libsnd name, recovered from Runtime Library 2.6's
 * LIBSND.LIB export table, where SEQREAD.C's helpers are still separate symbols
 * because that build did not inline them.
 * Reached from SsSeqDispatchControlChange's `case 6:`. Applies the pending
 * RPN (unk29 == 2) or NRPN (unk2a == 2) to the channel's VAB program by
 * rewriting the VagAtr of every tone.
 */
static inline s32 SsSeqCheckDataEntryValue(s32 data_entry_value) {
    switch (data_entry_value) {
    case 0:
        return 0;
    case 10:
        return 0;
    default:
        return 0;
    }
}

void ContDataEntry(s16 seq, s16 sep, u8 value) {
    s32 mask;
    ProgAtr program_attr;
    VagAtr tone_attr;
    SndAdsr adsr;
    SeqStruct *score;
    s32 tone;
    u8 channel;

    mask = 0xFF;
    score = &g_SndSeqTable[seq][sep];
    channel = score->channel;
    SsUtGetProgAtr(score->unk4c, score->programs[channel], &program_attr);
    if (score->unk27 == 1 && score->unk10 == 0) {
        score->unk28 = value;
        score->unk10 = 1;
        score->delta_value = SsSeqReadDeltaTime(seq, sep);
    } else if (score->unk29 == 2) {
        if (score->unk13 == 0 && score->play_mode == 0) {
            tone = 0;
            if (tone < (program_attr.tones + value) - value) {
                do {
                    s32 bend = value & 0x7F;

                    SsUtGetVagAtr(score->unk4c, score->programs[channel], (s16)tone,
                                 &tone_attr);
                    tone_attr.pbmin = tone_attr.pbmax = bend;
                    SsUtSetVagAtr(score->unk4c, score->programs[channel], (s16)tone,
                                 &tone_attr);
                    tone++;
                } while (tone < program_attr.tones);
            }
        }
        if (score->unk13 == 1 && score->play_mode == 0) {
            s32 shift;

            if (value > 0x40 && value < 0x80) {
                shift = SsSeqCheckDataEntryValue(value);
                shift =
                    ((((value) * 100) / 0x2000) * 0x2000) & 0xE000;
            } else {
                shift = 0;
            }
            tone = 0;
            if (tone < (program_attr.tones + value) - value) {
                do {
                    SsUtGetVagAtr(
                        score->unk4c, score->programs[channel], (s16)tone, &tone_attr);
                    tone_attr.shift += shift;
                    SsUtSetVagAtr(
                        score->unk4c, score->programs[channel], (s16)tone, &tone_attr);
                    tone++;
                } while (tone < program_attr.tones);
            }
        }
        if (score->unk13 == 2 && score->play_mode == 0) {
            s32 center;

            if (value >= 0x40 && value < 0x80) {
                center = (value * 100) * 0x40;
            } else {
                center = 0;
            }
            tone = 0;
            if (tone < (program_attr.tones + value) - value) {
                do {
                    SsUtGetVagAtr(
                        score->unk4c, score->programs[channel], (s16)tone, &tone_attr);
                    tone_attr.center += center;
                    SsUtSetVagAtr(
                        score->unk4c, score->programs[channel], (s16)tone, &tone_attr);
                    tone++;
                } while (tone < program_attr.tones);
            }
        }
        score->delta_value = SsSeqReadDeltaTime(seq, sep);
        score->unk29 = 0;
    } else if (score->unk2a == 2) {
        if (score->unk16 == 0x10) {
            for (tone = 0; tone < program_attr.tones; tone++) {
                SsSeqApplyNrpn(
                    score->unk4c,
                    score->programs[channel],
                    tone,
                    tone_attr,
                    adsr,
                    score->unk15,
                    value);
            }
        } else {
            SsSeqApplyNrpn(
                score->unk4c,
                score->programs[channel],
                score->unk16,
                tone_attr,
                adsr,
                score->unk15,
                value);
        }
        score->delta_value = SsSeqReadDeltaTime(seq, sep);
        score->unk2a = 0;
    } else {
        score->delta_value = SsSeqReadDeltaTime(seq, sep);
    }
}
