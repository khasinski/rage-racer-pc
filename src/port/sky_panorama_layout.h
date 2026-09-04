#ifndef RAGE_SKY_PANORAMA_LAYOUT_H
#define RAGE_SKY_PANORAMA_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

enum {
    RAGE_SKY_MAP_ROWS = 5,
    RAGE_SKY_MAP_COLUMNS = 16,
    RAGE_SKY_TILE_COUNT = 8,
    RAGE_SKY_PANORAMA_ROWS = 2,
};

int RageSkyPanoramaTile(
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS],
    int rowBase, int panoramaRow, int column);
int RageSkyExpandPanorama(
    uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase);

#endif
