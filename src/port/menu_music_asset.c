#include "menu_music_asset.h"

#include <string.h>

static uint32_t Read32Le(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t Read24Be(const uint8_t *p) {
    return (uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2];
}

int MenuMusicAssetOpen(const void *data, size_t size, MenuMusicAsset *asset) {
    const uint8_t *bytes = data;
    uint32_t headerOffset;
    uint32_t sequenceOffset;
    uint32_t samplesOffset;
    const uint8_t *sequence;

    if (bytes == NULL || asset == NULL || size < 25) return 0;
    headerOffset = Read32Le(bytes);
    sequenceOffset = Read32Le(bytes + 4);
    samplesOffset = Read32Le(bytes + 8);
    if (headerOffset < 12 || sequenceOffset <= headerOffset ||
        samplesOffset <= sequenceOffset || samplesOffset > size ||
        sequenceOffset + 15 > size) return 0;
    sequence = bytes + sequenceOffset;
    if (memcmp(sequence, "pQES", 4) != 0 || sequence[7] != 1) return 0;

    *asset = (MenuMusicAsset){
        .header = {bytes + headerOffset, sequenceOffset - headerOffset},
        .sequence = {sequence, samplesOffset - sequenceOffset},
        .samples = {bytes + samplesOffset, size - samplesOffset},
        .division = (uint32_t)sequence[8] << 8 | sequence[9],
        .tempoUs = Read24Be(sequence + 10),
    };
    return asset->division != 0 && asset->tempoUs != 0;
}

static int ReadVariableLength(const uint8_t **cursor, const uint8_t *end,
                              uint32_t *value) {
    uint32_t result = 0;
    unsigned count = 0;

    do {
        uint8_t byte;
        if (*cursor == end || count++ == 4) return 0;
        byte = *(*cursor)++;
        result = result << 7 | (byte & 0x7f);
        if (!(byte & 0x80)) {
            *value = result;
            return 1;
        }
    } while (1);
}

static int SkipSequenceEvent(const uint8_t **cursor, const uint8_t *end,
                             uint8_t *runningStatus, uint32_t *tempoUs,
                             int *ended) {
    uint8_t status;
    uint8_t data0;
    unsigned dataBytes;

    if (*cursor == end) return 0;
    status = *(*cursor)++;
    if (status < 0x80) {
        if (*runningStatus < 0x80 || *runningStatus >= 0xf0) return 0;
        data0 = status;
        status = *runningStatus;
    } else {
        *runningStatus = status;
        if (*cursor == end) return 0;
        data0 = *(*cursor)++;
    }

    if ((status & 0xf0) < 0xf0) {
        dataBytes = ((status & 0xf0) == 0xc0 ||
                     (status & 0xf0) == 0xd0) ? 0 : 1;
        if ((size_t)(end - *cursor) < dataBytes) return 0;
        *cursor += dataBytes;
        return 1;
    }

    if (status == 0xff) {
        uint32_t length;
        if (!ReadVariableLength(cursor, end, &length) ||
            length > (size_t)(end - *cursor)) return 0;
        if (data0 == 0x51) {
            if (length != 3) return 0;
            *tempoUs = Read24Be(*cursor);
            if (*tempoUs == 0) return 0;
        }
        if (data0 == 0x2f) *ended = 1;
        *cursor += length;
        return 1;
    }

    if (status == 0xf0 || status == 0xf7) {
        uint32_t length;
        /* PsyQ consumes the byte after status before reading this length. */
        const uint8_t *lengthCursor = *cursor - 1;
        if (!ReadVariableLength(&lengthCursor, end, &length) ||
            length > (size_t)(end - lengthCursor)) return 0;
        *cursor = lengthCursor + length;
        return 1;
    }
    return 0;
}

int MenuMusicSequenceTicks(const MenuMusicAsset *asset, unsigned tickRate,
                           size_t *tickCount) {
    const uint8_t *cursor;
    const uint8_t *end;
    uint32_t delta;
    uint32_t tempoUs;
    uint64_t accumulator = 0;
    uint8_t runningStatus = 0;
    size_t ticks = 0;

    if (asset == NULL || tickCount == NULL || tickRate == 0 ||
        asset->sequence.data == NULL || asset->sequence.size < 16 ||
        asset->division == 0 || asset->tempoUs == 0) return 0;
    cursor = asset->sequence.data + 15;
    end = asset->sequence.data + asset->sequence.size;
    tempoUs = asset->tempoUs;
    if (!ReadVariableLength(&cursor, end, &delta)) return 0;

    while (ticks < (size_t)tickRate * 60 * 60) {
        accumulator += (uint64_t)asset->division * 1000000;
        ticks++;
        while (accumulator >= (uint64_t)tempoUs * tickRate) {
            int ended = 0;
            accumulator -= (uint64_t)tempoUs * tickRate;
            if (delta > 0) delta--;
            while (delta == 0) {
                if (!SkipSequenceEvent(&cursor, end, &runningStatus,
                                       &tempoUs, &ended)) return 0;
                if (ended) {
                    *tickCount = ticks;
                    return 1;
                }
                if (!ReadVariableLength(&cursor, end, &delta)) return 0;
            }
        }
    }
    return 0;
}
