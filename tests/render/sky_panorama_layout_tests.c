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
    return 0;
}
