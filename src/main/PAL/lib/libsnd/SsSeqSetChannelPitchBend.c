#include "psyq/snd.h"


#include "psyq/snd_internal.h"

void SsSeqSetChannelPitchBend(long seq, long sep, long pitch, long amount) {
    long seq_raw = seq;
    long pitchRaw = pitch;
    long seq_offset = (seq_raw << 16) >> 14;
    long sep_s = (sep << 16) >> 16;
    long offset = (((((sep_s * 2) + sep_s) * 4) - sep_s) * 4) - sep_s;
    SeqStruct *state = (SeqStruct *)((u_char *)*(SeqStruct **)((u_char *)g_SndSeqTable + seq_offset) + (offset * 4));
    u_char channel = state->channel;
    SeqStruct *channel_state = (SeqStruct *)((u_char *)state + channel);
    u_char pan = channel_state->panpot[0];
    long bend = amount & 0xFF;

    if (((state->padAA >> channel) & 1) == 0 && state->left_volume != 0) {
        if ((u_char)amount != 0) {
            SpuVmSeKeyOn((short)(seq_raw | (sep << 8)), state->unk4c, channel_state->programs[0], (u_char)pitchRaw, bend, pan);
            state->padA8 = bend;
        } else {
            SpuVmSeKeyOff((short)(seq_raw | (sep << 8)), state->unk4c, channel_state->programs[0], (u_char)pitchRaw);
        }
    }
}


void SsSeqSetNoteParam2C(long seq, long sep, u_char value) {
    SeqStruct *state = &g_SndSeqTable[(short)seq][(short)sep];

    state->programs[state->channel] = value;
    state->delta_value = SsSeqReadDeltaTime((short)seq, (short)sep);
}
