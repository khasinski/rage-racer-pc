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
        sequenceOffset + 13 > size) return 0;
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
