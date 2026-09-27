#include "mod_selection.h"
#include <string.h>

static int Same(const char *a, const char *b) {
    return a && b && !strcmp(a,b);
}
static int Visit(const RageModSelectionEntry *entries, size_t count, size_t index,
                 unsigned char *state, RageModSelectionOrder *out) {
    if (state[index] == 2) return 1;
    state[index] = 1;
    const RageModSelectionEntry *entry = &entries[index];
    for (size_t r = 0; r < entry->dependencyCount; ++r) {
        const RageModDependency *dep = &entry->dependencies[r];
        size_t match = count;
        out->modIndex = index; out->dependencyIndex = r;
        for (size_t i = 0; i < count; ++i) {
            const char *id = dep->identity == RAGE_MOD_PACKAGE_ID
                ? entries[i].packageId : entries[i].manifestId;
            if (!Same(dep->id,id) || !Same(entry->region,entries[i].region) ||
                (dep->version && !Same(dep->version,entries[i].version))) continue;
            if (match != count) {
                out->code = RAGE_MOD_SELECTION_AMBIGUOUS;
                out->error = "multiple matching providers"; return 0;
            }
            match = i;
        }
        if (match == count) {
            out->code = RAGE_MOD_SELECTION_MISSING;
            out->error = "missing matching provider"; return 0;
        }
        if (state[match] == 1) {
            out->code = RAGE_MOD_SELECTION_CYCLE;
            out->error = "mod dependency cycle"; return 0;
        }
        if (!Visit(entries,count,match,state,out)) return 0;
    }
    state[index] = 2;
    out->indices[out->count++] = index;
    return 1;
}
int ModSelectionBuildOrder(const RageModSelectionEntry *entries, size_t count,
                          RageModSelectionOrder *out) {
    unsigned char state[RAGE_MOD_SELECTION_LIMIT] = {0};
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    out->modIndex = out->dependencyIndex = (size_t)-1;
    out->error = "invalid mod selection";
    out->code = RAGE_MOD_SELECTION_INVALID;
    if (count > RAGE_MOD_SELECTION_LIMIT || (count && !entries)) return 0;
    for (size_t i = 0; i < count; ++i) {
        const RageModSelectionEntry *e = &entries[i];
        out->modIndex = i;
        if (!e->region || e->dependencyCount > RAGE_MOD_DEPENDENCY_LIMIT ||
            (e->dependencyCount && !e->dependencies)) return 0;
        for (size_t r = 0; r < e->dependencyCount; ++r) {
            const RageModDependency *d = &e->dependencies[r];
            out->dependencyIndex = r;
            if (!d->id || !*d->id || (d->identity != RAGE_MOD_PACKAGE_ID &&
                d->identity != RAGE_MOD_MANIFEST_ID)) return 0;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        if (!Visit(entries,count,i,state,out)) {
            memset(out->indices,0,sizeof(out->indices)); out->count = 0;
            return 0;
        }
    }
    out->error = NULL;
    out->code = RAGE_MOD_SELECTION_OK;
    out->modIndex = out->dependencyIndex = (size_t)-1;
    return 1;
}
