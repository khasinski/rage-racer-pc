#include "mod_provider.h"
#include <string.h>

static int Unique(const char *const *ids, size_t count) {
    if (count && !ids) return 0;
    for (size_t i = 0; i < count; ++i) {
        if (!ids[i] || !*ids[i]) return 0;
        for (size_t j = 0; j < i; ++j)
            if (!strcmp(ids[i],ids[j])) return 0;
    }
    return 1;
}
RageModProviderResult ModProviderResolve(const char *const *candidates, size_t count,
    const char *winner, const char *const *previous, size_t previousCount,
    size_t *selected) {
    if (!selected) return RAGE_MOD_PROVIDER_INVALID;
    *selected = (size_t)-1;
    if (!count || count > RAGE_MOD_PROVIDER_LIMIT ||
        previousCount > RAGE_MOD_PROVIDER_LIMIT || !Unique(candidates,count) ||
        !Unique(previous,previousCount)) return RAGE_MOD_PROVIDER_INVALID;
    if (count == 1) { *selected = 0; return RAGE_MOD_PROVIDER_OK; }
    if (!winner || !*winner) return RAGE_MOD_PROVIDER_UNRESOLVED;
    if (count != previousCount) return RAGE_MOD_PROVIDER_STALE;
    size_t chosen = count;
    for (size_t i = 0; i < count; ++i) {
        size_t j;
        for (j = 0; j < previousCount; ++j)
            if (!strcmp(candidates[i],previous[j])) break;
        if (j == previousCount) return RAGE_MOD_PROVIDER_STALE;
        if (!strcmp(candidates[i],winner)) chosen = i;
    }
    if (chosen == count) return RAGE_MOD_PROVIDER_STALE;
    *selected = chosen;
    return RAGE_MOD_PROVIDER_OK;
}
