#include "common.h"
#include "game/track_internal.h"

#include <stdio.h>

CellScanOffsetTable g_CellScanOffsets;

static int s_failures;

static void ExpectOffset(const char *what, s32 direction, s32 rearView,
                         s32 wantX, s32 wantZ) {
    s32 offset[2];

    GetVisibleCellScanOffset(direction, 3, rearView, offset);
    if (offset[0] == wantX && offset[1] == wantZ) return;
    printf("FAIL %s: got %d,%d, expected %d,%d\n", what,
           offset[0], offset[1], wantX, wantZ);
    s_failures++;
}

int main(void) {
    s32 invalidOffset[2] = {99, 99};
    s32 direction;

    for (direction = 0; direction < CELL_SCAN_BASE_DIRECTION_COUNT;
         direction++) {
        g_CellScanOffsets.values[direction][3][0] = (s8)(direction + 1);
        g_CellScanOffsets.values[direction][3][1] = (s8)(direction + 11);
    }

    ExpectOffset("front start", 0, 0, 1, 11);
    ExpectOffset("front end", 7, 0, 8, 18);
    ExpectOffset("right start", 8, 0, 9, -19);
    ExpectOffset("right end", 15, 0, 2, -12);
    ExpectOffset("back start", 16, 0, -1, -11);
    ExpectOffset("back end", 23, 0, -8, -18);
    ExpectOffset("left start", 24, 0, -9, 19);
    ExpectOffset("left end", 31, 0, -2, 12);
    ExpectOffset("rear-view reflection", 24, 1, 9, -19);
    ExpectOffset("direction wraps", 32, 0, 1, 11);
    ExpectOffset("negative direction wraps", -1, 0, -2, 12);

    GetVisibleCellScanOffset(0, -1, 0, invalidOffset);
    if (invalidOffset[0] != 0 || invalidOffset[1] != 0) {
        puts("FAIL negative cell index was not rejected");
        s_failures++;
    }
    invalidOffset[0] = 99;
    invalidOffset[1] = 99;
    GetVisibleCellScanOffset(0, VISIBLE_CELL_COUNT, 0, invalidOffset);
    if (invalidOffset[0] != 0 || invalidOffset[1] != 0) {
        puts("FAIL cell index beyond scan table was not rejected");
        s_failures++;
    }
    GetVisibleCellScanOffset(0, 0, 0, NULL);

    if (s_failures != 0) {
        printf("visible_cell_scan: %d failures\n", s_failures);
        return 1;
    }
    printf("visible_cell_scan: ok\n");
    return 0;
}
