#ifndef RAGE_MOD_SELECTION_H
#define RAGE_MOD_SELECTION_H
#include <stddef.h>

enum { RAGE_MOD_SELECTION_LIMIT = 128, RAGE_MOD_DEPENDENCY_LIMIT = 48 };
typedef enum RageModIdentity { RAGE_MOD_PACKAGE_ID, RAGE_MOD_MANIFEST_ID } RageModIdentity;
typedef enum RageModSelectionError {
    RAGE_MOD_SELECTION_OK, RAGE_MOD_SELECTION_INVALID,
    RAGE_MOD_SELECTION_MISSING, RAGE_MOD_SELECTION_AMBIGUOUS,
    RAGE_MOD_SELECTION_CYCLE
} RageModSelectionError;
typedef struct RageModDependency {
    RageModIdentity identity;
    const char *id;
    const char *version; /* NULL accepts any version. */
} RageModDependency;
typedef struct RageModSelectionEntry {
    const char *packageId, *manifestId, *version, *region;
    const RageModDependency *dependencies;
    size_t dependencyCount;
} RageModSelectionEntry;
typedef struct RageModSelectionOrder {
    size_t indices[RAGE_MOD_SELECTION_LIMIT], count;
    size_t modIndex, dependencyIndex;
    const char *error; /* Static diagnostic; NULL on success. */
    RageModSelectionError code;
} RageModSelectionOrder;
/* Borrowed immutable descriptors. Requirements match identity namespace,
 * region and optional exact version. Multiple matching providers are an
 * error; unrelated duplicate display/manifest IDs do not create an edge.
 * Stable dependency-first output; failure publishes no partial order. */
int ModSelectionBuildOrder(const RageModSelectionEntry *entries, size_t count,
                          RageModSelectionOrder *out);
#endif
