/*
 * Which cell of the visibility mask a world point reads.
 *
 * The rule is two lines long and was copied by hand into three places, which
 * is exactly the kind of thing that stays right until one copy is edited. It
 * is also not the rule that fills the mask: BuildVisibleCells sets bits by
 * plain division, while the drawing code reads them back through a bias that
 * lands a point in the cell whose centre is nearest. The two differ by half a
 * cell on purpose, so the boundary sits at 1024 rather than at 2048, and that
 * is what the cases below pin.
 *
 * Negative coordinates are the part worth testing. An arithmetic shift rounds
 * towards minus infinity where a division rounds towards zero, and the second
 * bias exists to keep both sides of the origin numbering their cells the same
 * way.
 */

#include "common.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

u32 *g_VisibleCellMask;

static u32 s_mask[32];
static int s_failures;

static void Expect(const char *what, int got, int want) {
    if (got == want) return;
    printf("FAIL %s: got %d, expected %d\n", what, got, want);
    s_failures++;
}

static void ClearMask(void) { memset(s_mask, 0, sizeof(s_mask)); }

/* Marks the cell at column/row, which is how BuildVisibleCells writes them. */
static void SetCell(int column, int row) { s_mask[row] |= 1u << column; }

int main(void) {
    g_VisibleCellMask = s_mask;

    ClearMask();
    SetCell(0, 0);
    Expect("origin is in cell 0,0", TrackCellVisible(0, 0), 1);
    Expect("just inside the first cell", TrackCellVisible(1023, 1023), 1);
    Expect("the boundary is at 1024, not 2048",
           TrackCellVisible(1024, 0), 0);
    Expect("a point past the boundary in z", TrackCellVisible(0, 1024), 0);

    /*
     * The two biases are not the same number, so cell zero reaches further
     * back than forward: it holds everything from -2048 to 1023, half a cell
     * wider than any other. That asymmetry is the rule, and it is the part
     * worth writing down, because nothing about the two lines says it.
     */
    Expect("just below the origin", TrackCellVisible(-1, -1), 1);
    Expect("a cell's width back", TrackCellVisible(-2048, -2048), 1);
    Expect("the last coordinate cell zero holds",
           TrackCellVisible(-2049, -2049), 1);

    ClearMask();
    SetCell(1, 1);
    Expect("the next cell along", TrackCellVisible(1024, 1024), 1);
    Expect("its far corner", TrackCellVisible(3071, 3071), 1);
    Expect("one past it", TrackCellVisible(3072, 3072), 0);

    /*
     * Further back than -3071 the rule produces a negative cell, which is off
     * the grid the mask describes: it covers thirty-two cells of positive
     * space and the game never asks behind them. Nothing here asks either,
     * because the answer would come from a shift by a negative count.
     */

    /* A cleared mask hides everything, whatever the coordinate. */
    ClearMask();
    Expect("nothing is visible in an empty mask",
           TrackCellVisible(0, 0) | TrackCellVisible(5000, 5000), 0);

    if (s_failures != 0) {
        printf("visible_cell: %d failures\n", s_failures);
        return 1;
    }
    printf("visible_cell: ok\n");
    return 0;
}
