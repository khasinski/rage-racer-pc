#include "native_visibility.h"
#include <stddef.h>

int NativeVisibilityAllowsCell(const uint16_t *grid,
                              const uint32_t (*visibility)[32],
                              float cameraX, float cameraZ,
                              int cellX, int cellZ) {
    if (grid == NULL || visibility == NULL) return 1;
    if ((unsigned)cellX >= 32 || (unsigned)cellZ >= 32) return 0;
    /* Range checks precede float-to-int conversion, including NaN/infinity. */
    if (!(cameraX >= 0 && cameraX < 65536 &&
          cameraZ >= 0 && cameraZ < 65536)) return 1;
    unsigned x = (unsigned)cameraX / 2048;
    unsigned z = (unsigned)cameraZ / 2048;
    unsigned region = grid[z * 32 + x] >> 10;
    if (region >= 32) return 0;
    return (visibility[cellZ][cellX] & (UINT32_C(1) << region)) != 0;
}
