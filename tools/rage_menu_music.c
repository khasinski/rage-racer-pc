#include "menu_music_asset.h"

#include <libsnd.h>
#include <psyz/spu.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    SAMPLE_RATE = 44100,
    CHANNELS = 2,
    SEQUENCE_VOICES = 18,
    SEQUENCE_SPU_ADDRESS = 0x20000,
    MASTER_VOLUME = 0x3fff,
    REVERB_PRESET = 2,
    REVERB_DEPTH = 0x28,
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

static uint8_t *ReadFile(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    long length;
    uint8_t *data;
    if (file == NULL || fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        if (file != NULL) fclose(file);
        return NULL;
    }
    data = malloc((size_t)length);
    if (data == NULL || fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (size_t)length;
    return data;
}

static int Render(const MenuMusicAsset *asset, unsigned tickRate,
                  const char *outputPath) {
    unsigned char sequenceTable[512] = {0};
    int16_t samples[SAMPLE_RATE / 50 * CHANNELS];
    size_t ticks;
    uint64_t frameCount;
    unsigned framesPerTick;
    short vab;
    short sequence;
    FILE *output;

    /* Rage services PAL once per 50 Hz frame, but PsyQ's manual SEQ clock is
     * 60 Hz. The PAL disc's 1.2x tempo compensates for that service rate. */
    if (!MenuMusicSequenceTicks(asset, 60, &ticks) ||
        SAMPLE_RATE % tickRate != 0) return 0;
    framesPerTick = SAMPLE_RATE / tickRate;
    frameCount = ticks * (uint64_t)framesPerTick;
    if (frameCount > (UINT32_MAX - 36) / (CHANNELS * sizeof(int16_t))) return 0;

    Psyz_SpuInit();
    _SsInit();
    SsSetTableSize((char *)sequenceTable, 1, 1);
    SsSetTickMode(SS_NOTICK | 60);
    SsSetReservedVoice(SEQUENCE_VOICES);
    SsSetMVol(MASTER_VOLUME, MASTER_VOLUME);
    SsUtSetReverbType(REVERB_PRESET);
    SsUtReverbOn();
    SsUtSetReverbDepth(REVERB_DEPTH, REVERB_DEPTH);
    vab = SsVabOpenHeadSticky((unsigned char *)asset->header.data, -1,
                              SEQUENCE_SPU_ADDRESS);
    if (vab < 0 || SsVabTransBody((unsigned char *)asset->samples.data, vab) < 0 ||
        !SsVabTransCompleted(0)) return 0;
    sequence = SsSeqOpen((unsigned long *)asset->sequence.data, vab);
    if (sequence < 0) return 0;
    SsSeqSetVol(sequence, 127, 127);
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
            return 0;
        }
    }
    return fclose(output) == 0;
}

int main(int argc, char **argv) {
    MenuMusicAsset asset;
    size_t size;
    uint8_t *data;
    unsigned tickRate;
    int ok;

    if (argc != 4 || (argv[2][0] != '5' && argv[2][0] != '6')) {
        fprintf(stderr, "usage: %s SELBGM.BIN 50|60 OUTPUT.wav\n", argv[0]);
        return 2;
    }
    tickRate = (unsigned)strtoul(argv[2], NULL, 10);
    if (tickRate != 50 && tickRate != 60) return 2;
    data = ReadFile(argv[1], &size);
    if (data == NULL || !MenuMusicAssetOpen(data, size, &asset)) {
        fprintf(stderr, "invalid menu music asset: %s\n", argv[1]);
        free(data);
        return 1;
    }
    ok = Render(&asset, tickRate, argv[3]);
    free(data);
    if (!ok) fprintf(stderr, "could not render menu music\n");
    return ok ? 0 : 1;
}
