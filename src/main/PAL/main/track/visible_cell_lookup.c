#include "game/render_internal.h"
#include "game/track.h"

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
int TrackCellVisible(s32 x, s32 z) {
    s32 row = z + 0x400;
    s32 column = x + 0x400;

    if (row < 0) row = z + 0xBFF;
    if (column < 0) column = x + 0xBFF;
    return (g_VisibleCellMask[row >> 11] & (1 << (column >> 11))) != 0;
}
