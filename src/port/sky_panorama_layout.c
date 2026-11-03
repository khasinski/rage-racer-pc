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
    uint32_t width, uint32_t sourceHeight,
    const RageSkyPanoramaLayout *layout, int tileColumns) {
    enum { BYTES = 4 };
    uint32_t tileWidth;
    uint32_t tileRows;
    size_t stripBytes;
    int panoramaRow;
    int column;
    uint32_t row;

    if (destination == NULL || source == NULL || layout == NULL ||
        width < 8u || sourceHeight == 0u || (width % 8u) != 0u ||
        tileColumns <= 0 || (8 % tileColumns) != 0) {
        return 0;
    }
    tileWidth = width / 8u;
    if ((size_t)width > SIZE_MAX / (size_t)sourceHeight ||
        (size_t)width * (size_t)sourceHeight > SIZE_MAX / (2u * BYTES)) {
        return 0;
    }
    stripBytes = (size_t)width * (size_t)sourceHeight * BYTES;
    if (destinationSize < stripBytes * 2u || sourceSize < stripBytes) {
        return 0;
    }
    tileRows = (uint32_t)(sourceSize /
                          ((size_t)tileColumns * (size_t)tileWidth *
                           (size_t)sourceHeight * BYTES));
    /* Validate everything before touching output, including externally supplied
     * layouts from future replay/import consumers. Tiles must also index the
     * source atlas this call was sized for, or the per-row memcpy below would
     * read past the source buffer. */
    for (panoramaRow = 0; panoramaRow < RAGE_SKY_PANORAMA_ROWS; ++panoramaRow)
        for (column = 0; column < 8; ++column) {
            int tile = layout->tiles[panoramaRow][column];

            if (tile < 0 || tile >= RAGE_SKY_TILE_COUNT ||
                (uint32_t)(tile / tileColumns) >= tileRows) {
                return 0;
            }
        }
    for (panoramaRow = 0; panoramaRow < RAGE_SKY_PANORAMA_ROWS;
         panoramaRow++) {
        for (column = 0; column < 8; column++) {
            int tile = layout->tiles[panoramaRow][column];
            for (row = 0; row < sourceHeight; row++) {
                memcpy(destination +
                           (((size_t)panoramaRow * sourceHeight + row) *
                                width +
                            (size_t)column * tileWidth) * BYTES,
                       source + (((size_t)(tile / tileColumns) * sourceHeight + row) *
                                     ((size_t)tileColumns * tileWidth) +
                                 (size_t)(tile % tileColumns) * tileWidth) * BYTES,
                       (size_t)tileWidth * BYTES);
            }
        }
    }
    return 1;
}

int RageSkyExpandPanoramaLayoutScaled(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, uint32_t width, uint32_t sourceHeight,
    const RageSkyPanoramaLayout *layout) {
    return ExpandAtlas(destination, destinationSize, source, sourceSize,
                       width, sourceHeight, layout, 8);
}

int RageSkyExpandPanoramaLayout(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, const RageSkyPanoramaLayout *layout) {
    return ExpandAtlas(destination, destinationSize, source, sourceSize,
                       512u, 128u, layout, 8);
}

int RageSkyExpandTexturePageLayout(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, const RageSkyPanoramaLayout *layout) {
    return ExpandAtlas(destination, destinationSize, source, sourceSize,
                       512u, 128u, layout, 4);
}

int RageSkyExpandTexturePageLayoutScaled(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, uint32_t pageWidth,
    const RageSkyPanoramaLayout *layout) {
    if (pageWidth < 256u || pageWidth % 256u != 0u ||
        pageWidth > UINT32_MAX / 2u) return 0;
    return ExpandAtlas(destination, destinationSize, source, sourceSize,
                       pageWidth * 2u, pageWidth / 2u, layout, 4);
}

int RageSkyExpandPanorama(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase) {
    RageSkyPanoramaLayout layout;
    RageSkyCapturePanoramaLayout(&layout, map, rowBase);
    return RageSkyExpandPanoramaLayout(destination, destinationSize,
                                      source, sourceSize, &layout);
}
