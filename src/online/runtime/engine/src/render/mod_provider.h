#ifndef RAGE_MOD_PROVIDER_H
#define RAGE_MOD_PROVIDER_H
#include <stddef.h>

enum { RAGE_MOD_PROVIDER_LIMIT = 128 };
typedef enum RageModProviderResult {
    RAGE_MOD_PROVIDER_OK, RAGE_MOD_PROVIDER_INVALID,
    RAGE_MOD_PROVIDER_UNRESOLVED, RAGE_MOD_PROVIDER_STALE
} RageModProviderResult;
/* A choice belongs to the exact set of providers shown when it was made,
 * not to their iteration order. A sole provider needs no stored decision.
 * Strings are borrowed; failure sets selected to SIZE_MAX and never chooses
 * an implicit last-writer winner. Duplicate/empty provider IDs are invalid. */
RageModProviderResult ModProviderResolve(const char *const *candidates, size_t count,
    const char *winner, const char *const *previous, size_t previousCount,
    size_t *selected);
#endif
