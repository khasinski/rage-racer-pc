#include "game/track_internal.h"

_Static_assert((CELL_SCAN_HEADING_COUNT & (CELL_SCAN_HEADING_COUNT - 1)) == 0,
               "heading count must remain a power of two");

enum {
    CELL_SCAN_QUADRANT_HEADING_COUNT = CELL_SCAN_HEADING_COUNT / 4,
    CELL_SCAN_HALF_HEADING_COUNT = CELL_SCAN_HEADING_COUNT / 2,
    CELL_SCAN_THREE_QUARTER_HEADING_COUNT =
        CELL_SCAN_QUADRANT_HEADING_COUNT * 3,
};

void GetVisibleCellScanOffset(s32 direction, s32 cellIndex, s32 rearView,
                              s32 offset[2]) {
    s32 scanDirection;
    s32 xSign;
    s32 zSign;

    if (offset == NULL) return;
    offset[0] = 0;
    offset[1] = 0;
    if ((u32)cellIndex >= VISIBLE_CELL_COUNT) return;

    direction &= CELL_SCAN_HEADING_COUNT - 1;
    if (direction < CELL_SCAN_QUADRANT_HEADING_COUNT) {
        scanDirection = direction;
        xSign = 1;
        zSign = 1;
    } else if (direction < CELL_SCAN_HALF_HEADING_COUNT) {
        scanDirection = CELL_SCAN_HALF_HEADING_COUNT - direction;
        xSign = 1;
        zSign = -1;
    } else if (direction < CELL_SCAN_THREE_QUARTER_HEADING_COUNT) {
        scanDirection = direction - CELL_SCAN_HALF_HEADING_COUNT;
        xSign = -1;
        zSign = -1;
    } else {
        scanDirection = CELL_SCAN_HEADING_COUNT - direction;
        /* Retail reflects this quadrant only for the rear-view pass. Using
         * those signs in the main view drops the cells ahead on the left. */
        xSign = rearView ? 1 : -1;
        zSign = rearView ? -1 : 1;
    }

    offset[0] = g_CellScanOffsets.values[scanDirection][cellIndex][0] * xSign;
    offset[1] = g_CellScanOffsets.values[scanDirection][cellIndex][1] * zSign;
}
