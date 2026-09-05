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

typedef struct RageRuntimeEnvironmentLocation {
    uint32_t width, height;
    const char *path;
    size_t pathLength;
} RageRuntimeEnvironmentLocation;
/* Legacy line format: KEY WIDTH HEIGHT RELATIVE_RGBA_PATH. No header required.
 * Validate once before accepting an asset root; reject duplicate keys, invalid
 * dimensions, overflow, extra fields and unsafe paths. Locations borrow text.
 * Find requires a previously validated, unchanged index; it does not inspect
 * records after the matching key. Validation does not open referenced files. */
int EnvironmentIndexValidate(const char *text, size_t size, uint32_t maxDimension,
                             size_t *errorLine);
int EnvironmentIndexFind(const char *text, size_t size, uint32_t assetKey,
    uint32_t maxDimension, RageRuntimeEnvironmentLocation *out);

/* Return the declared cache contract version, or zero for an absent or
 * malformed header. Runtime consumers use this to reject caches whose
 * material variant layout predates their importer contract. */
uint32_t RuntimeIndexVersion(const char *text, size_t size);
/* Validate the complete current-version index, including duplicate key/set
 * pairs. errorLine is 1-based (0 on success or allocation failure). Does not
 * open referenced files. Intended once at asset-session initialization. */
int RuntimeIndexValidate(const char *text, size_t size, size_t *errorLine);

/* Parse the line-oriented `runtime-index.txt` produced by assetbrowser.
 * Pointers in `out` borrow `text`, which lets the platform asset cache decide
 * where and how to perform actual file I/O. Matching records must contain
 * exactly four fields and relative file paths without traversal/control bytes.
 * This is lexical validation, not symlink containment. Failure clears out. */
int RuntimeIndexFind(const char *text, size_t size, uint32_t assetKey,
                         RageRenderAssetSet assetSet,
                         RageRuntimeAssetLocation *out);

#endif
