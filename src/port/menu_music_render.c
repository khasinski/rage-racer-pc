#include "menu_music_render.h"

#include <libsnd.h>
#include <libspu.h>
#include <psyz/spu.h>

#include <stdint.h>
#include <stdio.h>

enum {
    SAMPLE_RATE = 44100,
    CHANNELS = 2,
    SEQUENCE_VOICES = 18,
    SEQUENCE_SPU_ADDRESS = 0x20000,
    MASTER_VOLUME = 0x3fff,
    SEQUENCE_VOLUME = 96,
    PAL_TEMPO_US = 328947,
    NTSC_TEMPO_US = 394736,
};

extern void _SsInit(void);
extern void _SsVmFlush(void);

static void Write16Le(FILE *file, uint16_t value) {
    fputc(value & 0xff, file);
    fputc(value >> 8, file);
}

static void Write32Le(FILE *file, uint32_t value) {
    Write16Le(file, (uint16_t)value);
    Write16Le(file, (uint16_t)(value >> 16));
}

static int WriteWavHeader(FILE *file, uint32_t frames) {
    uint32_t dataSize = frames * CHANNELS * sizeof(int16_t);
    if (fwrite("RIFF", 1, 4, file) != 4) return 0;
    Write32Le(file, 36 + dataSize);
    if (fwrite("WAVEfmt ", 1, 8, file) != 8) return 0;
    Write32Le(file, 16);
    Write16Le(file, 1);
    Write16Le(file, CHANNELS);
    Write32Le(file, SAMPLE_RATE);
    Write32Le(file, SAMPLE_RATE * CHANNELS * sizeof(int16_t));
    Write16Le(file, CHANNELS * sizeof(int16_t));
    Write16Le(file, 16);
    if (fwrite("data", 1, 4, file) != 4) return 0;
    Write32Le(file, dataSize);
    return !ferror(file);
}

unsigned MenuMusicTickRate(const MenuMusicAsset *asset) {
    if (asset == NULL) return 0;
    if (asset->tempoUs == PAL_TEMPO_US) return 50;
    if (asset->tempoUs == NTSC_TEMPO_US) return 60;
    return 0;
}

int MenuMusicRenderWav(const MenuMusicAsset *asset, unsigned tickRate,
                       const char *outputPath) {
    unsigned char sequenceTable[512] = {0};
    int16_t samples[SAMPLE_RATE / 50 * CHANNELS];
    size_t ticks;
    uint64_t frameCount;
    unsigned framesPerTick;
    short vab;
    short sequence;
    FILE *output;

    /* PsyQ's manual SEQ clock is 60 Hz. The PAL asset's tempo encodes the
     * original 50 Hz service rate, while output remains ordinary 44.1 kHz PCM. */
    if (!MenuMusicSequenceTicks(asset, 60, &ticks) ||
        (tickRate != 50 && tickRate != 60) || SAMPLE_RATE % tickRate != 0)
        return 0;
    framesPerTick = SAMPLE_RATE / tickRate;
    frameCount = ticks * (uint64_t)framesPerTick;
    if (frameCount > (UINT32_MAX - 36) / (CHANNELS * sizeof(int16_t))) return 0;

    Psyz_SpuInit();
    SpuInit();
    _SsInit();
    SsSetTableSize((char *)sequenceTable, 1, 1);
    SsSetTickMode(SS_NOTICK | 60);
    SsSetReservedVoice(SEQUENCE_VOICES);
    SsSetMVol(MASTER_VOLUME, MASTER_VOLUME);
    SsUtReverbOff();
    vab = SsVabOpenHeadSticky((unsigned char *)asset->header.data, -1,
                              SEQUENCE_SPU_ADDRESS);
    if (vab < 0 || SsVabTransBody((unsigned char *)asset->samples.data, vab) < 0 ||
        !SsVabTransCompleted(0)) return 0;
    sequence = SsSeqOpen((unsigned long *)asset->sequence.data, vab);
    if (sequence < 0) return 0;
    SsSeqSetVol(sequence, SEQUENCE_VOLUME, SEQUENCE_VOLUME);
    SsSeqPlay(sequence, SSPLAY_PLAY, 1);

    output = fopen(outputPath, "wb");
    if (output == NULL || !WriteWavHeader(output, (uint32_t)frameCount)) {
        if (output != NULL) fclose(output);
        return 0;
    }
    for (size_t tick = 0; tick < ticks; tick++) {
        SsSeqCalledTbyT();
        _SsVmFlush();
        Psyz_SpuPullSamples(samples, (int)framesPerTick);
        if (fwrite(samples, sizeof(int16_t) * CHANNELS, framesPerTick, output) !=
            framesPerTick) {
            fclose(output);
            remove(outputPath);
            return 0;
        }
    }
    if (fclose(output) != 0) {
        remove(outputPath);
        return 0;
    }
    return 1;
}
