#include "game/render_internal.h"
#include "game/track.h"

#include <stdint.h>

enum { VISIBILITY_CELL_SIZE = 2048 };

/*
 * Whether the visibility mask holds the cell a world point falls in.
 *
 * The mask is one bit per 2048-unit cell: a word per row of z, a bit per
 * column of x. BuildVisibleCells fills it by plain division, but the drawing
 * code reads it back through the bias below, which lands a point in the cell
 * whose centre is nearest rather than the one it sits inside. The two rules
 * differ by half a cell, and that is retail's, not a mistake to tidy: the
 * shift also rounds towards minus infinity where a division would round
 * towards zero, so the bias is what keeps negative coordinates on the same
 * side of the grid as positive ones.
 *
 * Three places worked this out for themselves, in the same fifteen lines
 * each. They are the reason this is one function: a grid rule copied by hand
 * holds only until one copy is edited.
 */
static s32 VisibilityCellForCoordinate(s32 coordinate) {
    int64_t biased = (int64_t)coordinate + 0x400;

    if (biased < 0) biased = (int64_t)coordinate + 0xBFF;
    if (biased < 0 ||
        biased >= VISIBILITY_CELL_SIZE * TERRAIN_CELL_GRID_SIZE) return -1;
    return (s32)(biased >> 11);
}

int TrackCellVisible(s32 x, s32 z) {
    return CellVisibilityMaskContains(
        g_VisibleCellMask, VisibilityCellForCoordinate(x),
        VisibilityCellForCoordinate(z));
}
