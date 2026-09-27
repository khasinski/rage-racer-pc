/* Software SPU and the libsnd utility layer for the browser (see web_spu.h).
 *
 * Voice playback, the ADSR envelope, Gaussian interpolation and the mix follow
 * the desktop's PsyZ SPU (external/psyz/psyz/src/psyz/psyz_spu.c); key-on
 * volume, pan and pitch follow the decompiled libsnd the desktop links
 * (decomp/src/libsnd/vm_nowon.c, vm_n2p.c, ut_keyv.c, ut_key.c) and PsyZ's
 * libsnd.c (SsUtPitchBend, SsUtChangePitch, SsUtSetVVol, _SsVmAlloc). Samples
 * are read from the VAB bodies directly instead of an emulated SPU RAM. */
#include "web_spu.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/snd.h"
#include "../engine/external/psyz/psyz/src/psyz/spu_gauss_exact.h"


enum {
    ADPCM_BLOCK_BYTES = 16,
    ADPCM_BLOCK_SAMPLES = 28,
    VAB_HEADER_SIZE = 32,
    VAB_PROGRAM_SIZE = 16,
    VAB_TONE_SIZE = 32,
    VAB_TONES_PER_PROGRAM = 16,
    VAB_LENGTH_ENTRIES = 256,
    /* libsnd marks voices keyed by the utility calls with this sep number. */
    UTILITY_OWNER = 0x21,
    /* SsSetReservedVoice(8) in InitEffectVoiceRuntime: SsUtKeyOn picks among
     * the first eight voices. */
    ALLOCATABLE_VOICES = 8,
    /* PsyZ: latency before a keyed envelope starts. */
    KEY_ON_DELAY_SAMPLES = 6,
    MAX_PITCH = 0x4000,
    MAX_VOICE_VOLUME = 0x3FFF,
    MAIN_VOLUME = 0x3FFF, /* SsSetMVol(0x3FFF, 0x3FFF) */
};

typedef enum { ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE, ADSR_OFF } AdsrState;

typedef struct WebVab {
    int open;
    uint8_t *header;
    size_t headerSize;
    uint8_t *body;
    size_t bodySize;
    int programLimit, programCount, vagCount;
    uint8_t ordinal[128];
    uint32_t vagOffset[VAB_LENGTH_ENTRIES], vagSize[VAB_LENGTH_ENTRIES];
} WebVab;

typedef struct Voice {
    /* libsnd bookkeeping */
    int owner, vab, prog, tone, note, prior;
    unsigned age;
    /* registers */
    int pitch, left, right;
    uint16_t adsr1, adsr2;
    /* playback */
    const uint8_t *sample;
    uint32_t sampleSize, cur, repeat;
    int repeatSet, primedLoop;
    int16_t hist1, hist2, decoded[ADPCM_BLOCK_SAMPLES];
    uint8_t flags, index, needsDecode, active;
    uint32_t spos;
    int16_t window[4];
    uint8_t windowPos;
    int envelope;
    unsigned envelopeCounter;
    AdsrState envelopeState;
    int keyOff, delay;
} Voice;

static WebVab s_vabs[WEB_SPU_VAB_SLOTS];
static Voice s_voices[WEB_SPU_VOICES];

static const uint16_t kPitchTable[] = {
    0x1000, 0x100E, 0x101D, 0x102C, 0x103B, 0x104A, 0x1059, 0x1068, 0x1078,
    0x1087, 0x1096, 0x10A5, 0x10B5, 0x10C4, 0x10D4, 0x10E3, 0x10F3, 0x1103,
    0x1113, 0x1122, 0x1132, 0x1142, 0x1152, 0x1162, 0x1172, 0x1182, 0x1193,
    0x11A3, 0x11B3, 0x11C4, 0x11D4, 0x11E5, 0x11F5, 0x1206, 0x1216, 0x1227,
    0x1238, 0x1249, 0x125A, 0x126B, 0x127C, 0x128D, 0x129E, 0x12AF, 0x12C1,
    0x12D2, 0x12E3, 0x12F5, 0x1306, 0x1318, 0x132A, 0x133C, 0x134D, 0x135F,
    0x1371, 0x1383, 0x1395, 0x13A7, 0x13BA, 0x13CC, 0x13DE, 0x13F1, 0x1403,
    0x1416, 0x1428, 0x143B, 0x144E, 0x1460, 0x1473, 0x1486, 0x1499, 0x14AC,
    0x14BF, 0x14D3, 0x14E6, 0x14F9, 0x150D, 0x1520, 0x1534, 0x1547, 0x155B,
    0x156F, 0x1583, 0x1597, 0x15AB, 0x15BF, 0x15D3, 0x15E7, 0x15FB, 0x1610,
    0x1624, 0x1638, 0x164D, 0x1662, 0x1676, 0x168B, 0x16A0, 0x16B5, 0x16CA,
    0x16DF, 0x16F4, 0x170A, 0x171F, 0x1734, 0x174A, 0x175F, 0x1775, 0x178B,
    0x17A1, 0x17B6, 0x17CC, 0x17E2, 0x17F9, 0x180F, 0x1825, 0x183B, 0x1852,
    0x1868, 0x187F, 0x1896, 0x18AC, 0x18C3, 0x18DA, 0x18F1, 0x1908, 0x191F,
    0x1937, 0x194E, 0x1965, 0x197D, 0x1995, 0x19AC, 0x19C4, 0x19DC, 0x19F4,
    0x1A0C, 0x1A24, 0x1A3C, 0x1A55, 0x1A6D, 0x1A85, 0x1A9E, 0x1AB7, 0x1ACF,
    0x1AE8, 0x1B01, 0x1B1A, 0x1B33, 0x1B4C, 0x1B66, 0x1B7F, 0x1B98, 0x1BB2,
    0x1BCC, 0x1BE5, 0x1BFF, 0x1C19, 0x1C33, 0x1C4D, 0x1C67, 0x1C82, 0x1C9C,
    0x1CB7, 0x1CD1, 0x1CEC, 0x1D07, 0x1D22, 0x1D3D, 0x1D58, 0x1D73, 0x1D8E,
    0x1DA9, 0x1DC5, 0x1DE0, 0x1DFC, 0x1E18, 0x1E34, 0x1E50, 0x1E6C, 0x1E88,
    0x1EA4, 0x1EC1, 0x1EDD, 0x1EFA, 0x1F16, 0x1F33, 0x1F50, 0x1F6D, 0x1F8A,
    0x1FA7, 0x1FC5, 0x1FE2, 0x2000,
};

static uint16_t Read16(const uint8_t *bytes) { return (uint16_t)(bytes[0] | bytes[1] << 8); }

static int16_t Clamp16(int value) {
    return (int16_t)(value < -32768 ? -32768 : value > 32767 ? 32767 : value);
}

/* ---- banks ---------------------------------------------------------------- */

static const WebVab *Vab(int vabId) {
    return vabId >= 0 && vabId < WEB_SPU_VAB_SLOTS && s_vabs[vabId].open ? &s_vabs[vabId] : NULL;
}

static const uint8_t *Program(const WebVab *vab, int prog) {
    return prog >= 0 && prog < vab->programLimit ? vab->header + VAB_HEADER_SIZE + prog * VAB_PROGRAM_SIZE : NULL;
}

/* The VagAtr for a program's tone; libsnd indexes tones by the program's
 * position among the programs that have any (SsVabOpenHead's reserved1). */
static const uint8_t *Tone(const WebVab *vab, int prog, int tone) {
    const uint8_t *program = Program(vab, prog);
    if (!program || tone < 0 || tone >= VAB_TONES_PER_PROGRAM || vab->ordinal[prog] >= vab->programCount)
        return NULL;
    return vab->header + VAB_HEADER_SIZE + (size_t)vab->programLimit * VAB_PROGRAM_SIZE +
           ((size_t)vab->ordinal[prog] * VAB_TONES_PER_PROGRAM + (size_t)tone) * VAB_TONE_SIZE;
}

void WebSpuCloseVab(int vabId) {
    if (vabId < 0 || vabId >= WEB_SPU_VAB_SLOTS) return;
    for (int v = 0; v < WEB_SPU_VOICES; ++v)
        if (s_voices[v].vab == vabId && (s_voices[v].active || s_voices[v].owner)) memset(&s_voices[v], 0, sizeof(s_voices[v]));
    free(s_vabs[vabId].header);
    free(s_vabs[vabId].body);
    memset(&s_vabs[vabId], 0, sizeof(s_vabs[vabId]));
}

int WebSpuOpenVab(int vabId, const uint8_t *header, size_t headerSize, const uint8_t *body, size_t bodySize) {
    WebVab vab;
    size_t required, offset = 0;
    uint32_t version;
    int ordinal = 0;
    if (vabId < 0 || vabId >= WEB_SPU_VAB_SLOTS || !header || !body || headerSize < VAB_HEADER_SIZE ||
        header[0] != 'p' || header[1] != 'B' || header[2] != 'A' || header[3] != 'V') return -1;
    memset(&vab, 0, sizeof(vab));
    version = (uint32_t)Read16(header + 4) | (uint32_t)Read16(header + 6) << 16;
    vab.programLimit = version >= 5 ? 128 : 64;
    vab.programCount = Read16(header + 18);
    vab.vagCount = Read16(header + 22);
    if (vab.programCount > vab.programLimit || vab.vagCount >= VAB_LENGTH_ENTRIES) return -1;
    required = VAB_HEADER_SIZE + (size_t)vab.programLimit * VAB_PROGRAM_SIZE +
               (size_t)vab.programCount * VAB_TONES_PER_PROGRAM * VAB_TONE_SIZE + VAB_LENGTH_ENTRIES * 2;
    if (required > headerSize) return -1;
    for (int prog = 0; prog < vab.programLimit; ++prog) {
        vab.ordinal[prog] = (uint8_t)ordinal;
        if (header[VAB_HEADER_SIZE + prog * VAB_PROGRAM_SIZE] != 0) ++ordinal;
    }
    {
        const uint8_t *lengths = header + required - VAB_LENGTH_ENTRIES * 2;
        for (int vag = 0; vag <= vab.vagCount; ++vag) {
            const uint32_t size = (uint32_t)Read16(lengths + vag * 2) * (version >= 5 ? 8u : 4u);
            vab.vagOffset[vag] = (uint32_t)offset;
            vab.vagSize[vag] = size;
            offset += size;
        }
    }
    if (offset > bodySize) return -1;
    vab.header = malloc(headerSize);
    vab.body = malloc(bodySize ? bodySize : 1);
    if (!vab.header || !vab.body) {
        free(vab.header);
        free(vab.body);
        return -1;
    }
    memcpy(vab.header, header, headerSize);
    memcpy(vab.body, body, bodySize);
    vab.headerSize = headerSize;
    vab.bodySize = bodySize;
    vab.open = 1;
    WebSpuCloseVab(vabId);
    s_vabs[vabId] = vab;
    return vabId;
}

/* ---- libsnd maths ---------------------------------------------------------- */

/* vm_n2p.c note2pitch2 for one tone (center note, fine shift). */
static int NotePitch(const uint8_t *tone, int note, int fine) {
    int step = (fine + tone[5]) / 8, octaveBase = 0, semitones, octave, index, pitch;
    if (step >= 16) {
        octaveBase = 1;
        step -= 16;
    }
    semitones = (short)(octaveBase + (note + 60 - tone[4]));
    octave = semitones / 12;
    semitones -= octave * 12;
    if (semitones < 0) {
        semitones += 12;
        --octave;
    }
    index = semitones * 16 + step;
    if (index < 0) index = 0;
    if (index >= (int)(sizeof(kPitchTable) / sizeof(kPitchTable[0]))) index = (int)(sizeof(kPitchTable) / sizeof(kPitchTable[0])) - 1;
    pitch = kPitchTable[index];
    if (octave > 5) pitch <<= octave - 5 > 8 ? 8 : octave - 5;
    else if (octave < 5) pitch >>= 5 - octave > 16 ? 16 : 5 - octave;
    return pitch > MAX_PITCH ? MAX_PITCH : pitch;
}

/* vm_nowon.c _SsVmKeyOnNow: bank, program, tone and caller volume with the
 * three pan stages, then squared into the SPU's volume register range. */
static void KeyOnVolume(const WebVab *vab, const uint8_t *program, const uint8_t *tone,
                        int volL, int volR, int *left, int *right) {
    unsigned volume, pan, seed, chL, chR;
    const unsigned bankVolume = vab->header[24], toneVolume = tone[2], tonePan = tone[3];
    const unsigned programVolume = program[1], programPan = program[4];
    if (volL == volR) {
        volume = (unsigned)(volL < 0 ? 0 : volL);
        pan = 0x40;
    } else if (volR < volL) {
        volume = (unsigned)volL;
        pan = (unsigned)((volR * 0x40) / volL);
    } else {
        volume = (unsigned)volR;
        pan = (unsigned)(0x7F - (volL * 0x40) / volR);
    }
    seed = (unsigned)((uint64_t)volume * bankVolume * 16383 / 16129);
    seed = (unsigned)((uint64_t)seed * programVolume * toneVolume / 16129);
    chL = chR = seed;
    if (tonePan < 64) chR = chR * tonePan / 63;
    else chL = chL * (127 - tonePan) / 63;
    if (programPan < 64) chR = chR * programPan / 63;
    else chL = chL * (127 - programPan) / 63;
    if (pan < 64) chR = chR * pan / 63;
    else chL = chL * (127 - pan) / 63;
    chL = (unsigned)((uint64_t)chL * chL / 16383);
    chR = (unsigned)((uint64_t)chR * chR / 16383);
    *left = chL > MAX_VOICE_VOLUME ? MAX_VOICE_VOLUME : (int)chL;
    *right = chR > MAX_VOICE_VOLUME ? MAX_VOICE_VOLUME : (int)chR;
}

/* ---- voices ----------------------------------------------------------------- */

static void StartVoice(int index, const WebVab *vab, int vabId, int prog, int tone,
                       const uint8_t *attributes, int note, int fine, int volL, int volR) {
    Voice *voice = &s_voices[index];
    const int vag = (int16_t)Read16(attributes + 22);
    memset(voice, 0, sizeof(*voice));
    voice->owner = UTILITY_OWNER;
    voice->vab = vabId;
    voice->prog = prog;
    voice->tone = tone;
    voice->note = note;
    voice->prior = attributes[0];
    voice->pitch = NotePitch(attributes, note, fine);
    KeyOnVolume(vab, Program(vab, prog), attributes, volL, volR, &voice->left, &voice->right);
    voice->adsr1 = Read16(attributes + 16);
    voice->adsr2 = Read16(attributes + 18);
    if (vag <= 0 || vag > vab->vagCount || vab->vagSize[vag] < ADPCM_BLOCK_BYTES) return;
    voice->sample = vab->body + vab->vagOffset[vag];
    voice->sampleSize = vab->vagSize[vag];
    /* PsyZ spu_key_on_voice: the engine layers (voices 14..17) loop END+REPEAT
     * samples back to their own start; other voices do not. */
    voice->primedLoop = index >= 14 && index <= 17;
    voice->index = ADPCM_BLOCK_SAMPLES;
    voice->needsDecode = 1;
    voice->spos = 0x10000;
    voice->active = 1;
    voice->envelopeState = ADSR_ATTACK;
    voice->delay = KEY_ON_DELAY_SAMPLES;
}

static int ValidVoice(long voice) { return voice >= 0 && voice < WEB_SPU_VOICES; }

long SsUtKeyOnV(long voice, long vabId, long prog, long tone, long note, long fine, long volL, long volR) {
    const WebVab *vab = Vab((int)vabId);
    const uint8_t *attributes;
    if (!ValidVoice(voice) || !vab || !(attributes = Tone(vab, (int)prog, (int)tone)) ||
        Read16(attributes + 22) == 0) return -1;
    StartVoice((int)voice, vab, (int)vabId, (int)prog, (int)tone, attributes, (int)note, (int)fine,
               (int)volL, (int)volR);
    return voice;
}

/* _SsVmAlloc: a free voice, else the lowest priority, then the oldest. */
static int AllocateVoice(int prior) {
    int selected = -1;
    for (int v = 0; v < ALLOCATABLE_VOICES; ++v) {
        if (!s_voices[v].active) {
            selected = v;
            break;
        }
    }
    if (selected < 0) {
        int threshold = prior;
        unsigned oldest = 0;
        for (int v = 0; v < ALLOCATABLE_VOICES; ++v) {
            if (s_voices[v].prior < threshold || (s_voices[v].prior == threshold && s_voices[v].age >= oldest)) {
                threshold = s_voices[v].prior;
                oldest = s_voices[v].age;
                selected = v;
            }
        }
    }
    if (selected >= 0)
        for (int v = 0; v < ALLOCATABLE_VOICES; ++v) s_voices[v].age++;
    return selected;
}

short SsUtKeyOn(short vabId, short prog, short tone, short note, short fine, short volL, short volR) {
    const WebVab *vab = Vab(vabId);
    const uint8_t *attributes;
    int voice;
    if (!vab || !(attributes = Tone(vab, prog, tone)) || Read16(attributes + 22) == 0) return -1;
    voice = AllocateVoice(attributes[0]);
    if (voice < 0) return -1;
    StartVoice(voice, vab, vabId, prog, tone, attributes, note, fine, volL, volR);
    s_voices[voice].age = 0;
    return (short)voice;
}

long SsUtKeyOffV(long voice) {
    if (!ValidVoice(voice)) return -1;
    s_voices[voice].keyOff = 1;
    s_voices[voice].owner = 0;
    return 0;
}

/* PsyZ SsUtSetVVol: linear 0..127 levels into the volume register. */
short SsUtSetVVol(short voice, short left, short right) {
    if (!ValidVoice(voice)) return -1;
    s_voices[voice].left = left * 0x81 > MAX_VOICE_VOLUME ? MAX_VOICE_VOLUME : left < 0 ? 0 : left * 0x81;
    s_voices[voice].right = right * 0x81 > MAX_VOICE_VOLUME ? MAX_VOICE_VOLUME : right < 0 ? 0 : right * 0x81;
    return 0;
}

static long Retune(long voice, long vabId, long prog, int note, int fine) {
    const WebVab *vab = Vab((int)vabId);
    const uint8_t *attributes;
    Voice *target;
    if (!ValidVoice(voice)) return -1;
    target = &s_voices[voice];
    if (target->owner != UTILITY_OWNER || target->vab != vabId || target->prog != prog || !vab ||
        !(attributes = Tone(vab, (int)prog, target->tone))) return -1;
    target->pitch = NotePitch(attributes, note, fine);
    return 0;
}

/* PsyZ: only the pitch register changes; the voice keeps its key-on note. */
long SsUtChangePitch(long voice, long vabId, long prog, long oldNote, long oldFine, long newNote, long newFine) {
    (void)oldFine;
    if (!ValidVoice(voice) || s_voices[voice].note != oldNote) return -1;
    return Retune(voice, vabId, prog, (int)newNote, (int)newFine);
}

/* 64 is centre; each side scales by the tone's VAB bend limit. */
long SsUtPitchBend(long voice, long vabId, long prog, long note, long pbend) {
    const WebVab *vab = Vab((int)vabId);
    const uint8_t *attributes;
    int bend, base, fine = 0;
    (void)note;
    if (!ValidVoice(voice) || s_voices[voice].owner != UTILITY_OWNER || !vab ||
        !(attributes = Tone(vab, (int)prog, s_voices[voice].tone))) return -1;
    bend = (int)pbend - 64;
    base = s_voices[voice].note;
    if (bend > 0) {
        const int scaled = bend * attributes[13];
        base += scaled / 63;
        fine = (scaled % 63) * 2;
    } else if (bend < 0) {
        const int scaled = bend * attributes[12];
        const int quotient = scaled >= 0 ? scaled / 64 : -((-scaled + 63) / 64);
        base += quotient - 1;
        fine = (scaled - quotient * 64) * 2 + 127;
    }
    return Retune(voice, vabId, prog, base, fine);
}

/* libspu: SPU_OFF, SPU_ON, or SPU_OFF_ENV_ON while a released voice fades. */
long SpuGetKeyStatus(u_long voiceBit) {
    for (int v = 0; v < WEB_SPU_HARDWARE_VOICES; ++v) {
        if (voiceBit != (1ul << v)) continue;
        if (!s_voices[v].active || s_voices[v].envelopeState == ADSR_OFF) return 0;
        return s_voices[v].keyOff ? 3 : 1;
    }
    return 0;
}

void WebSpuSilence(void) { memset(s_voices, 0, sizeof(s_voices)); }

void WebSpuReset(void) {
    WebSpuSilence();
    for (int vab = 0; vab < WEB_SPU_VAB_SLOTS; ++vab) WebSpuCloseVab(vab);
}

/* ---- playback (psyz_spu.c) ------------------------------------------------ */

static void DecodeBlock(Voice *voice, const uint8_t *block) {
    static const int positive[5] = {0, 60, 115, 98, 122};
    static const int negative[5] = {0, 0, -52, -55, -60};
    const int shiftIn = block[0] & 0x0F, shift = shiftIn > 12 ? 9 : 12 - shiftIn;
    int filter = (block[0] >> 4) & 0x07;
    int previous = voice->hist1, previous2 = voice->hist2;
    if (filter > 4) filter = 4;
    voice->flags = block[1];
    for (int i = 0; i < 14; ++i) {
        for (int n = 0; n < 2; ++n) {
            int nibble = (block[2 + i] >> (n * 4)) & 0x0F;
            if (nibble & 8) nibble -= 16;
            const int value = nibble * (1 << shift) + ((previous * positive[filter]) >> 6) +
                              ((previous2 * negative[filter]) >> 6);
            voice->decoded[i * 2 + n] = Clamp16(value);
            previous2 = previous;
            previous = voice->decoded[i * 2 + n];
        }
    }
    voice->hist1 = (int16_t)previous;
    voice->hist2 = (int16_t)previous2;
}

static void Stop(Voice *voice) {
    voice->active = 0;
    voice->window[voice->windowPos] = 0;
    voice->windowPos = (voice->windowPos + 1) & 3;
}

static int DecodeOne(Voice *voice) {
    if (voice->index >= ADPCM_BLOCK_SAMPLES) {
        if (!voice->needsDecode && (voice->flags & 0x01)) {
            /* END: repeat to the loop start the sample marked, or (engine
             * layers) to its own start; otherwise the voice is done. */
            if (!(voice->flags & 0x02) || (!voice->repeatSet && !voice->primedLoop)) {
                Stop(voice);
                return 0;
            }
            voice->cur = voice->repeatSet ? voice->repeat : 0;
        }
        voice->needsDecode = 1;
        voice->index = 0;
    }
    if (voice->needsDecode) {
        const uint8_t *block;
        int sentinel;
        if (voice->cur + ADPCM_BLOCK_BYTES > voice->sampleSize) {
            Stop(voice);
            return 0;
        }
        block = voice->sample + voice->cur;
        /* Sony's one-shot terminator (00 07, then 0x77s) is silence. */
        sentinel = block[0] == 0 && (block[1] & 0x07) == 0x07;
        for (int i = 2; sentinel && i < ADPCM_BLOCK_BYTES; ++i) sentinel = block[i] == 0x77;
        if (sentinel) {
            Stop(voice);
            return 0;
        }
        DecodeBlock(voice, block);
        if (voice->flags & 0x04) {
            voice->repeat = voice->cur;
            voice->repeatSet = 1;
        }
        voice->cur += ADPCM_BLOCK_BYTES;
        voice->needsDecode = 0;
    }
    voice->window[voice->windowPos] = voice->decoded[voice->index++];
    voice->windowPos = (voice->windowPos + 1) & 3;
    return 1;
}

static int VoiceSample(Voice *voice) {
    const unsigned step = voice->pitch ? (unsigned)voice->pitch << 4 : 1;
    unsigned phase;
    int sum;
    while (voice->spos >= 0x10000) {
        if (!DecodeOne(voice)) {
            voice->spos = 0;
            break;
        }
        voice->spos -= 0x10000;
    }
    phase = (voice->spos >> 8) & 0xFF;
    sum = spu_gauss_exact[0x0FF - phase] * voice->window[voice->windowPos & 3] +
          spu_gauss_exact[0x1FF - phase] * voice->window[(voice->windowPos + 1) & 3] +
          spu_gauss_exact[0x100 + phase] * voice->window[(voice->windowPos + 2) & 3] +
          spu_gauss_exact[0x000 + phase] * voice->window[(voice->windowPos + 3) & 3];
    voice->spos += step;
    return Clamp16(sum >> 15);
}

static unsigned RateDenominator(int rate) { return rate < 48 ? 1u : 1u << ((rate >> 2) - 11); }
static int RateIncrease(int rate) { return rate < 48 ? (7 - (rate & 3)) << (11 - (rate >> 2)) : 7 - (rate & 3); }
static int RateDecrease(int rate) { return rate < 48 ? (-8 + (rate & 3)) << (11 - (rate >> 2)) : -8 + (rate & 3); }

static void StepEnvelope(Voice *voice) {
    unsigned counter;
    if (voice->delay) {
        voice->delay--;
        return;
    }
    if (voice->keyOff && voice->envelopeState != ADSR_OFF) voice->envelopeState = ADSR_RELEASE;
    counter = ++voice->envelopeCounter;
#define FIRES(rate) ((counter % RateDenominator(rate)) == 0)
    switch (voice->envelopeState) {
    case ADSR_ATTACK: {
        int rate = (voice->adsr1 >> 8) & 0x7F;
        if (((voice->adsr1 >> 15) & 1) && voice->envelope >= 0x6000) rate += 8;
        if (FIRES(rate)) {
            voice->envelope += RateIncrease(rate);
            if (voice->envelope >= 0x7FFF) {
                voice->envelope = 0x7FFF;
                voice->envelopeState = ADSR_DECAY;
            }
        }
        break;
    }
    case ADSR_DECAY: {
        const int rate = ((voice->adsr1 >> 4) & 0x0F) * 4;
        if (FIRES(rate)) {
            voice->envelope += (RateDecrease(rate) * voice->envelope) >> 15;
            if (voice->envelope < 0) voice->envelope = 0;
            if (((voice->envelope >> 11) & 0xF) <= (voice->adsr1 & 0x0F)) voice->envelopeState = ADSR_SUSTAIN;
        }
        break;
    }
    case ADSR_SUSTAIN: {
        int rate = (voice->adsr2 >> 6) & 0x7F;
        const int decrease = (voice->adsr2 >> 14) & 1, exponential = (voice->adsr2 >> 15) & 1;
        if (!decrease) {
            if (exponential && voice->envelope >= 0x6000) rate += 8;
            if (FIRES(rate)) {
                voice->envelope += RateIncrease(rate);
                if (voice->envelope > 0x7FFF) voice->envelope = 0x7FFF;
            }
        } else if (FIRES(rate)) {
            voice->envelope += exponential ? (RateDecrease(rate) * voice->envelope) >> 15 : RateDecrease(rate);
            if (voice->envelope < 0) voice->envelope = 0;
        }
        break;
    }
    case ADSR_RELEASE: {
        const int rate = (voice->adsr2 & 0x1F) * 4;
        if (FIRES(rate)) {
            voice->envelope += (voice->adsr2 >> 5) & 1 ? (RateDecrease(rate) * voice->envelope) >> 15
                                                       : RateDecrease(rate);
            if (voice->envelope <= 0) {
                voice->envelope = 0;
                voice->envelopeState = ADSR_OFF;
                voice->active = 0;
            }
        }
        break;
    }
    case ADSR_OFF:
        break;
    }
#undef FIRES
}

void WebSpuRender(int16_t *out, int frames) {
    for (int frame = 0; frame < frames; ++frame) {
        int left = 0, right = 0;
        for (int v = 0; v < WEB_SPU_VOICES; ++v) {
            Voice *voice = &s_voices[v];
            int sample;
            if (!voice->active) continue;
            sample = VoiceSample(voice);
            StepEnvelope(voice);
            if (voice->envelopeState == ADSR_OFF) sample = 0;
            sample = (sample * voice->envelope) >> 15;
            left += (sample * voice->left) >> 14;
            right += (sample * voice->right) >> 14;
        }
        out[frame * 2] = Clamp16((left * MAIN_VOLUME) >> 14);
        out[frame * 2 + 1] = Clamp16((right * MAIN_VOLUME) >> 14);
    }
}
