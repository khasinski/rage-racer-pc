#ifndef RAGE_NATIVE_VISIBILITY_H
#define RAGE_NATIVE_VISIBILITY_H

#include <stdint.h>

/* Original game coordinates, before the native renderer's Z sign conversion.
 * These tables describe authored visibility, independent of camera heading.
 * Missing tables or a camera outside their domain leave the scene unfiltered. */
int NativeVisibilityAllowsCell(const uint16_t *grid,
                              const uint32_t (*visibility)[32],
                              float cameraX, float cameraZ,
                              int cellX, int cellZ);

#endif
