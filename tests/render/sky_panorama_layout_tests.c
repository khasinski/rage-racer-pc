#include "sky_panorama_layout.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static const int16_t s_retailMap[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS] = {
    {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7},
    {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7},
    {4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3},
    {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7},
    {4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3},
};

int main(void) {
    static uint8_t source[512 * 128 * 4];
    static uint8_t expanded[512 * 256 * 4];
    int rowBase;
    int panoramaRow;
    int column;

    /* Compare every classic map lookup used by the four authored row bases
     * with the corresponding native panorama cell. */
    for (rowBase = 0; rowBase <= RAGE_SKY_MAP_ROWS - 2; rowBase++) {
        for (panoramaRow = 0; panoramaRow < 2; panoramaRow++) {
            for (column = -16; column < 32; column++) {
                assert(RageSkyPanoramaTile(s_retailMap, rowBase,
                                           panoramaRow, column) ==
                       s_retailMap[rowBase + panoramaRow]
                                  [(unsigned)column & 15]);
            }
        }
    }

    /* Course 0 uses rows 0/1: both halves have the unshifted panorama. */
    assert(RageSkyPanoramaTile(s_retailMap, 0, 0, 0) == 0);
    assert(RageSkyPanoramaTile(s_retailMap, 0, 1, 4) == 4);

    /* The course-2 and attract variants use alternating rows. These were
     * flattened to row zero by the modern importer, rotating every other
     * cloud band through half a turn. */
    assert(RageSkyPanoramaTile(s_retailMap, 1, 0, 0) == 0);
    assert(RageSkyPanoramaTile(s_retailMap, 1, 1, 0) == 4);
    assert(RageSkyPanoramaTile(s_retailMap, 3, 0, 7) == 7);
    assert(RageSkyPanoramaTile(s_retailMap, 3, 1, 7) == 3);

    assert(RageSkyPanoramaTile(s_retailMap, 3, 1, 16) == 4);
    assert(RageSkyPanoramaTile(s_retailMap, -1, 0, 0) == 0);
    assert(RageSkyPanoramaTile(s_retailMap, 4, 1, 0) == 0);

    for (column = 0; column < 8; column++) {
        int row;
        for (row = 0; row < 128; row++) {
            memset(source + ((row * 512 + column * 64) * 4), column,
                   64 * 4);
        }
    }
    assert(RageSkyExpandPanorama(expanded, sizeof(expanded), source,
                                 sizeof(source), s_retailMap, 1));
    assert(expanded[0] == 0);
    assert(expanded[(128 * 512) * 4] == 4);
    assert(!RageSkyExpandPanorama(expanded, 1, source, sizeof(source),
                                  s_retailMap, 1));
    {
        int16_t mutableMap[RAGE_SKY_MAP_ROWS][RAGE_SKY_MAP_COLUMNS];
        RageSkyPanoramaLayout captured;
        static uint8_t expected[sizeof(expanded)];
        memcpy(mutableMap, s_retailMap, sizeof(mutableMap));
        assert(RageSkyExpandPanorama(expected, sizeof(expected), source,
                                    sizeof(source), mutableMap, 1));
        RageSkyCapturePanoramaLayout(&captured, mutableMap, 1);
        memset(mutableMap, 0, sizeof(mutableMap));
        assert(RageSkyExpandPanoramaLayout(expanded, sizeof(expanded), source,
                                          sizeof(source), &captured));
        assert(memcmp(expected, expanded, sizeof(expanded)) == 0);
        captured.tiles[1][7] = 8;
        memset(expanded, 0xA5, sizeof(expanded));
        assert(!RageSkyExpandPanoramaLayout(expanded, sizeof(expanded), source,
                                           sizeof(source), &captured));
        for (size_t i = 0; i < sizeof(expanded); ++i) assert(expanded[i] == 0xA5);
        RageSkyCapturePanoramaLayout(&captured, NULL, -1);
        for (size_t i = 0; i < sizeof(captured.tiles); ++i)
            assert(((const uint8_t *)captured.tiles)[i] == 0);
    }
    {
        static uint8_t page[256 * 256 * 4];
        static uint8_t pageExpanded[sizeof(expanded)];
        RageSkyPanoramaLayout layout;
        for (unsigned tile = 0; tile < 8; ++tile) {
            for (unsigned y = 0; y < 128; ++y) {
                for (unsigned x = 0; x < 64; ++x) {
                    const uint8_t rgba[4] = {(uint8_t)x, (uint8_t)y, (uint8_t)tile, 255};
                    memcpy(source + (y * 512 + tile * 64 + x) * 4, rgba, 4);
                    memcpy(page + (((tile / 4) * 128 + y) * 256 +
                                   (tile % 4) * 64 + x) * 4, rgba, 4);
                }
            }
        }
        for (rowBase = 0; rowBase < 4; ++rowBase) {
            RageSkyCapturePanoramaLayout(&layout, s_retailMap, rowBase);
            assert(RageSkyExpandPanoramaLayout(expanded, sizeof(expanded),
                                               source, sizeof(source), &layout));
            assert(RageSkyExpandTexturePageLayout(pageExpanded, sizeof(pageExpanded),
                                                  page, sizeof(page), &layout));
            assert(memcmp(expanded, pageExpanded, sizeof(expanded)) == 0);
            for (unsigned y = 0; y < 256; ++y) {
                for (unsigned x = 0; x < 512; ++x) {
                    const uint8_t *pixel = pageExpanded + (y * 512 + x) * 4;
                    assert(pixel[0] == x % 64);
                    assert(pixel[1] == y % 128);
                    assert(pixel[2] == s_retailMap[rowBase + y / 128][x / 64]);
                    assert(pixel[3] == 255);
                }
            }
        }
        memset(pageExpanded, 0xA5, sizeof(pageExpanded));
        assert(!RageSkyExpandTexturePageLayout(pageExpanded, sizeof(pageExpanded),
                                               page, sizeof(page) - 1, &layout));
        assert(!RageSkyExpandTexturePageLayout(pageExpanded, sizeof(pageExpanded) - 1,
                                               page, sizeof(page), &layout));
        layout.tiles[1][7] = 8;
        assert(!RageSkyExpandTexturePageLayout(pageExpanded, sizeof(pageExpanded),
                                               page, sizeof(page), &layout));
        for (size_t i = 0; i < sizeof(pageExpanded); ++i) assert(pageExpanded[i] == 0xA5);
    }
    return 0;
}
