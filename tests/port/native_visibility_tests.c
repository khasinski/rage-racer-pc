#include "native_visibility.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    uint16_t grid[32 * 32] = {0};
    uint32_t visibility[32][32] = {{0}};
    /* Region words use the camera's direct row, while terrain mesh indices
     * elsewhere use the reversed row. Mixing those conventions hides scenery. */
    grid[2 * 32 + 1] = (uint16_t)((31u << 10) | 177u);
    grid[29 * 32 + 1] = 3u << 10;
    visibility[9][28] = UINT32_C(1) << 31;
    assert(NativeVisibilityAllowsCell(grid, visibility, 2048, 4096, 28, 9));
    assert(!NativeVisibilityAllowsCell(grid, visibility, 2047, 4096, 28, 9));
    assert(!NativeVisibilityAllowsCell(grid, visibility, 2048, 4095, 28, 9));
    assert(!NativeVisibilityAllowsCell(grid, visibility, 2048, 4096, 9, 28));
    grid[2 * 32 + 1] = 32u << 10;
    assert(!NativeVisibilityAllowsCell(grid, visibility, 2048, 4096, 28, 9));
    grid[2 * 32 + 1] = 63u << 10;
    assert(!NativeVisibilityAllowsCell(grid, visibility, 2048, 4096, 28, 9));
    grid[31 * 32 + 31] = 7u << 10;
    visibility[31][31] = 1u << 7;
    assert(NativeVisibilityAllowsCell(grid, visibility, 65535, 65535, 31, 31));
    assert(!NativeVisibilityAllowsCell(grid, visibility, 0, 0, -1, 0));
    assert(!NativeVisibilityAllowsCell(grid, visibility, 0, 0, 0, 32));
    assert(!NativeVisibilityAllowsCell(grid, visibility, 0, 0, INT_MAX, INT_MIN));
    assert(NativeVisibilityAllowsCell(NULL, visibility, 0, 0, 0, 0));
    assert(NativeVisibilityAllowsCell(grid, NULL, 0, 0, 0, 0));
    const float outside[] = {-1, 65536, NAN, INFINITY, -INFINITY};
    for (unsigned i = 0; i < sizeof(outside)/sizeof(outside[0]); ++i) {
        assert(NativeVisibilityAllowsCell(grid, visibility, outside[i], 0, 0, 0));
        assert(NativeVisibilityAllowsCell(grid, visibility, 0, outside[i], 0, 0));
    }
    puts("native authored-region boundaries passed");
    return 0;
}
