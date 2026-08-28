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

enum { RAGE_RUNTIME_INDEX_VERSION = 2 };

/* Return the declared cache contract version, or zero for an absent or
 * malformed header. Runtime consumers use this to reject caches whose
 * material variant layout predates their importer contract. */
uint32_t RuntimeIndexVersion(const char *text, size_t size);

/* Parse the line-oriented `runtime-index.txt` produced by assetbrowser.
 * Pointers in `out` borrow `text`, which lets the platform asset cache decide
 * where and how to perform actual file I/O. */
int RuntimeIndexFind(const char *text, size_t size, uint32_t assetKey,
                         RageRenderAssetSet assetSet,
                         RageRuntimeAssetLocation *out);

#endif
