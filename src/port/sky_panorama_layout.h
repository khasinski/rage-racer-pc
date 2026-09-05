#ifndef RAGE_SKY_PANORAMA_LAYOUT_H
#define RAGE_SKY_PANORAMA_LAYOUT_H

#include <stddef.h>
#include <stdint.h>
#include "../render/sky_layout.h"

enum {
    RAGE_SKY_MAP_ROWS = 5,
    RAGE_SKY_MAP_COLUMNS = 16,
    RAGE_SKY_TILE_COUNT = 8,
    RAGE_SKY_PANORAMA_ROWS = 2,
};

/* Resolves and copies authored selection. No pointers into game state survive. */
void RageSkyCapturePanoramaLayout(RageSkyPanoramaLayout *out,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase);
int RageSkyExpandPanoramaLayout(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, const RageSkyPanoramaLayout *layout);
/* Same copied layout, applied to the live importer's decoded 256x256 RGBA
 * texture page (four columns by two rows of tiles), not the 512x128 strip.
 * Source/destination must not overlap; neither buffer is retained. */
int RageSkyExpandTexturePageLayout(uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize, const RageSkyPanoramaLayout *layout);

int RageSkyPanoramaTile(
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS],
    int rowBase, int panoramaRow, int column);
int RageSkyExpandPanorama(
    uint8_t *destination, size_t destinationSize,
    const uint8_t *source, size_t sourceSize,
    const int16_t map[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS], int rowBase);

#endif
