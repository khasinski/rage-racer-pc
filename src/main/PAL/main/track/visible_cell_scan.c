#include "game/track_internal.h"

void GetVisibleCellScanOffset(s32 direction, s32 cellIndex, s32 rearView,
                              s32 offset[2]) {
    s32 scanDirection;
    s32 xSign;
    s32 zSign;

    direction &= CELL_SCAN_DIRECTION_COUNT - 1;
    if (direction < 8) {
        scanDirection = direction;
        xSign = 1;
        zSign = 1;
    } else if (direction < 16) {
        scanDirection = 16 - direction;
        xSign = 1;
        zSign = -1;
    } else if (direction < 24) {
        scanDirection = direction - 16;
        xSign = -1;
        zSign = -1;
    } else {
        scanDirection = 32 - direction;
        /* Retail reflects this quadrant only for the rear-view pass. Using
         * those signs in the main view drops the cells ahead on the left. */
        xSign = rearView ? 1 : -1;
        zSign = rearView ? -1 : 1;
    }

    offset[0] = g_CellScanOffsets.values[scanDirection][cellIndex][0] * xSign;
    offset[1] = g_CellScanOffsets.values[scanDirection][cellIndex][1] * zSign;
}
