#include "sky_panorama_layout.h"

#include <stddef.h>
#include <string.h>

int RageSkyPanoramaTile(
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS],
    int rowBase, int panoramaRow, int column) {
    int row;
    int tile;

    if (map == NULL || rowBase < 0 ||
        rowBase > RAGE_SKY_MAP_ROWS - RAGE_SKY_PANORAMA_ROWS ||
        panoramaRow < 0 || panoramaRow >= RAGE_SKY_PANORAMA_ROWS) {
        return 0;
    }
    row = rowBase + panoramaRow;
    tile = map[row][(unsigned)column & (RAGE_SKY_MAP_COLUMNS - 1)];
    return tile >= 0 && tile < RAGE_SKY_TILE_COUNT ? tile : 0;
}

int RageSkyExpandPanorama(
    uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase) {
    enum { WIDTH = 512, SOURCE_HEIGHT = 128, TILE_WIDTH = 64, BYTES = 4 };
    int panoramaRow;
    int column;
    int row;

    if (destination == NULL || source == NULL ||
        destinationSize < WIDTH * SOURCE_HEIGHT * 2u * BYTES ||
        sourceSize < WIDTH * SOURCE_HEIGHT * BYTES) {
        return 0;
    }
    for (panoramaRow = 0; panoramaRow < RAGE_SKY_PANORAMA_ROWS;
         panoramaRow++) {
        for (column = 0; column < 8; column++) {
            int tile = RageSkyPanoramaTile(map, rowBase, panoramaRow, column);
            for (row = 0; row < SOURCE_HEIGHT; row++) {
                memcpy(destination +
                           (((size_t)panoramaRow * SOURCE_HEIGHT + row) *
                                WIDTH +
                            (size_t)column * TILE_WIDTH) * BYTES,
                       source + ((size_t)row * WIDTH +
                                 (size_t)tile * TILE_WIDTH) * BYTES,
                       TILE_WIDTH * BYTES);
            }
        }
    }
    return 1;
}
