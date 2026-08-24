#ifndef RAGE_RMESH_INDEX_H
#define RAGE_RMESH_INDEX_H

#include <stddef.h>
#include <stdint.h>

#include "render_world.h"

typedef struct RageRuntimeAssetLocation {
    const char *meshPath;
    size_t meshPathLength;
    const char *materialPath;
    size_t materialPathLength;
} RageRuntimeAssetLocation;

/* Parse the line-oriented `runtime-index.txt` produced by assetbrowser.
 * Pointers in `out` borrow `text`, which lets the platform asset cache decide
 * where and how to perform actual file I/O. */
int RageRuntimeIndexFind(const char *text, size_t size, uint32_t assetKey,
                         RageRenderAssetSet assetSet,
                         RageRuntimeAssetLocation *out);

#endif

