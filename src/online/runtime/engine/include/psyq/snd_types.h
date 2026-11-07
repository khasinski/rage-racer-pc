#ifndef PSYQ_SND_TYPES_H
#define PSYQ_SND_TYPES_H

#include "common.h"

/*
 * The libsnd table layouts on their own, without the function prototypes.
 * Several libsnd translation units need the types but declare the surrounding
 * functions with narrower parameters than psyq/snd.h does -- retail's callers
 * and callees disagree, and both spellings are load-bearing -- so those files
 * include this header instead of the full one.
 */
typedef struct SeqStruct {
    u_char unk0;
    u_char pad1[3];
    u_char *read_pos;
    u_char *next_sep_pos;
    u_char *loop_pos;
    u_char unk10;
    u_char unk11;
    u_char channel;
    u_char unk13;
    u_char play_mode;
    u_char unk15;
    u_char unk16;
    u_char panpot[16];
    u_char unk27;
    u_char unk28;
    u_char unk29;
    u_char unk2a;
    u_char unk2b;
    u_char programs[16];
    u_char unk3C;
    u_char pad3D;
    short unk3E;
    short unk40;
    short unk42;
    short tempo_step;
    short unk46;
    u_short unk48;
    short tempo_multiplier;
    short unk4c;
    short vol[16];
    short unk6E;
    short tick_period;
    short unk72;
    u_short left_volume;
    u_short right_volume;
    short unk78;
    short unk7A;
    long base_delta_value;
    u_long unk80;
    long base_unk84;
    long delta_value;
    long tempo;
    long flags;
    u_long unk94;
    u_long unk98;
    long unk9C;
    long tempo_countdown;
    u_long target_tempo;
    short padA8;
    short padAA;
} SeqStruct;

typedef struct VabHdr {
    long form;
    long ver;
    long id;
    u_long fsize;
    u_short reserved0;
    u_short ps;
    u_short ts;
    u_short vs;
    u_char mvol;
    u_char pan;
    u_char attr1;
    u_char attr2;
    u_long reserved1;
} VabHdr;

typedef struct ProgAtr {
    u_char tones;
    u_char mvol;
    u_char prior;
    u_char mode;
    u_char mpan;
    signed char reserved0;
    short attr;
    u_long reserved1;
    u_long reserved2;
} ProgAtr;

typedef struct VagAtr {
    u_char prior;
    u_char mode;
    u_char vol;
    u_char pan;
    u_char center;
    u_char shift;
    u_char min;
    u_char max;
    u_char vibW;
    u_char vibT;
    u_char porW;
    u_char porT;
    u_char pbmin;
    u_char pbmax;
    u_char reserved1;
    u_char reserved2;
    u_short adsr1;
    u_short adsr2;
    short prog;
    u_short vag;
    short reserved[4];
} VagAtr;

typedef struct SndAdsr {
    u_short attack_mode;
    u_short attack_rate;
    u_short decay_rate;
    u_short sustain_level;
    u_short sustain_mode;
    short sustain_direction;
    short sustain_rate;
    u_short release_mode;
    short release_rate;
} SndAdsr;

/*
 * One libsnd voice slot (0x34 bytes). Seven translation units each carried
 * their own copy of this layout under a name ending in the address of the
 * function that introduced it; the offsets always agreed. SpuVmAutoVol's copy
 * named 0x1E..0x26 volumeStep / volumeCounter / volumeCounterReload /
 * currentVolume / targetVolume, which may be the better reading of the last
 * two -- the pan fields at 0x30/0x32 are what suggested start/end here.
 */
typedef struct SpuVoice {
    short vag;
    short age;
    short pitch;
    u_short env;
    short base_volume;
    signed char pan;
    signed char unkB;
    short note;
    short seq_sep;
    short program_index;
    short program;
    short tone;
    short vab_id;
    short priority;
    u_char pad1A;
    u_char active;
    short auto_volume;
    short volume_step;
    short volume_counter;
    short volume_counter_reload;
    short start_volume;
    short end_volume;
    short auto_pan;
    short pan_step;
    short pan_counter;
    short pan_counter_reload;
    short start_pan;
    short end_pan;
} SpuVoice;

/* The libsnd "current attributes" block the key-on path fills in. */
typedef struct SvmCurrentAttr {
    u_char tones;
    u_char vab_id;
    u_char note;
    u_char fine;
    u_char volume;
    u_char pan;
    u_char program;
    u_char program_index;
    u_char unk8;
    u_char unk9;
    u_char master_volume;
    u_char master_pan;
    u_char tone;
    u_char tone_volume;
    u_char tone_pan;
    u_char priority;
    u_char center;
    u_char shift;
    u_char min;
    u_char max;
    u_char mode;
    u_char pad15;
    short seq_sep;
    short vag;
    short voice;
    short register_offset;
    short tone_index;
} SvmCurrentAttr;


#endif
