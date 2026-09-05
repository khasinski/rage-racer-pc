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

void RageSkyCapturePanoramaLayout(RageSkyPanoramaLayout *out,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase) {
    if (out == NULL) return;
    for (int row = 0; row < RAGE_SKY_PANORAMA_ROWS; ++row)
        for (int column = 0; column < 8; ++column)
            out->tiles[row][column] = (uint8_t)RageSkyPanoramaTile(
                map, rowBase, row, column);
}

static int ExpandAtlas(
    uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize,
    const RageSkyPanoramaLayout *layout, int tileColumns) {
    enum { WIDTH = 512, SOURCE_HEIGHT = 128, TILE_WIDTH = 64, BYTES = 4 };
    int panoramaRow;
    int column;
    int row;

    if (destination == NULL || source == NULL || layout == NULL ||
        destinationSize < WIDTH * SOURCE_HEIGHT * 2u * BYTES ||
        sourceSize < WIDTH * SOURCE_HEIGHT * BYTES) {
        return 0;
    }
    /* Validate everything before touching output, including externally supplied
     * layouts from future replay/import consumers. */
    for (panoramaRow = 0; panoramaRow < RAGE_SKY_PANORAMA_ROWS; ++panoramaRow)
        for (column = 0; column < 8; ++column)
            if (layout->tiles[panoramaRow][column] >= RAGE_SKY_TILE_COUNT) return 0;
    for (panoramaRow = 0; panoramaRow < RAGE_SKY_PANORAMA_ROWS;
         panoramaRow++) {
        for (column = 0; column < 8; column++) {
            int tile = layout->tiles[panoramaRow][column];
            for (row = 0; row < SOURCE_HEIGHT; row++) {
                memcpy(destination +
                           (((size_t)panoramaRow * SOURCE_HEIGHT + row) *
                                WIDTH +
                            (size_t)column * TILE_WIDTH) * BYTES,
                       source + (((size_t)(tile / tileColumns) * SOURCE_HEIGHT + row) *
                                     ((size_t)tileColumns * TILE_WIDTH) +
                                 (size_t)(tile % tileColumns) * TILE_WIDTH) * BYTES,
                       TILE_WIDTH * BYTES);
            }
        }
    }
    return 1;
}

int RageSkyExpandPanoramaLayout(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, const RageSkyPanoramaLayout *layout) {
    return ExpandAtlas(destination, destinationSize, source, sourceSize, layout, 8);
}

int RageSkyExpandTexturePageLayout(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, const RageSkyPanoramaLayout *layout) {
    return ExpandAtlas(destination, destinationSize, source, sourceSize, layout, 4);
}

int RageSkyExpandPanorama(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase) {
    RageSkyPanoramaLayout layout;
    RageSkyCapturePanoramaLayout(&layout, map, rowBase);
    return RageSkyExpandPanoramaLayout(destination, destinationSize,
                                      source, sourceSize, &layout);
}
