#include "render/mod_provider.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const char *ids[] = {"base","addon","third"};
    const char *reverse[] = {"addon","base"};
    const char *duplicate[] = {"base","base"};
    const char *empty[] = {""};
    size_t selected = 42;
    assert(ModProviderResolve(ids,1,NULL,NULL,0,&selected) == RAGE_MOD_PROVIDER_OK && selected == 0);
    assert(ModProviderResolve(ids,2,NULL,NULL,0,&selected) == RAGE_MOD_PROVIDER_UNRESOLVED);
    assert(selected == (size_t)-1);
    assert(ModProviderResolve(ids,2,"addon",reverse,2,&selected) == RAGE_MOD_PROVIDER_OK && selected == 1);
    assert(ModProviderResolve(reverse,2,"addon",ids,2,&selected) == RAGE_MOD_PROVIDER_OK && selected == 0);
    assert(ModProviderResolve(ids,3,"addon",reverse,2,&selected) == RAGE_MOD_PROVIDER_STALE);
    assert(selected == (size_t)-1);
    assert(ModProviderResolve(ids,2,"third",reverse,2,&selected) == RAGE_MOD_PROVIDER_STALE);
    assert(ModProviderResolve(ids,2,"addon",ids+1,2,&selected) == RAGE_MOD_PROVIDER_STALE);
    assert(ModProviderResolve(duplicate,2,"base",ids,2,&selected) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(ids,2,"base",duplicate,2,&selected) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(empty,1,NULL,NULL,0,&selected) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(NULL,1,NULL,NULL,0,&selected) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(ids,0,NULL,NULL,0,&selected) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(ids,2,"base",NULL,2,&selected) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(ids,1,NULL,NULL,0,NULL) == RAGE_MOD_PROVIDER_INVALID);
    assert(ModProviderResolve(ids,RAGE_MOD_PROVIDER_LIMIT+1,NULL,NULL,0,&selected) == RAGE_MOD_PROVIDER_INVALID);
    char names[RAGE_MOD_PROVIDER_LIMIT][16];
    const char *all[RAGE_MOD_PROVIDER_LIMIT];
    for (size_t i=0;i<RAGE_MOD_PROVIDER_LIMIT;++i) {
        snprintf(names[i],sizeof(names[i]),"provider-%zu",i); all[i]=names[i];
    }
    assert(ModProviderResolve(all,RAGE_MOD_PROVIDER_LIMIT,all[127],all,
        RAGE_MOD_PROVIDER_LIMIT,&selected) == RAGE_MOD_PROVIDER_OK && selected == 127);
    puts("provider selection, stale choices, duplicate IDs and bounds passed");
    return 0;
}
