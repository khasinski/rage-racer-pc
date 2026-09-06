#include "render/mod_provider.h"
#include <assert.h>
#include <stdio.h>
#include "render/resource_provider.h"
typedef struct Probe { RageResourceStatus status; unsigned calls; } Probe;
static RageResourceStatus ResolveProbe(void *context) {
    Probe *probe=context;++probe->calls;return probe->status;
}
static void CheckResourceProviders(void) {
    Probe probes[]={{RAGE_RESOURCE_MISSING,0},{RAGE_RESOURCE_READY,0},{RAGE_RESOURCE_READY,0}};
    RageResourceProvider providers[]={{ResolveProbe,&probes[0]},{ResolveProbe,&probes[1]},{ResolveProbe,&probes[2]}};
    size_t selected=99;
    assert(ResourceProviderResolve(providers,3,&selected)==RAGE_RESOURCE_READY&&selected==1);
    assert(probes[0].calls==1&&probes[1].calls==1&&probes[2].calls==0);
    probes[0].status=RAGE_RESOURCE_ERROR;
    assert(ResourceProviderResolve(providers,3,&selected)==RAGE_RESOURCE_ERROR&&selected==3);
    assert(probes[1].calls==1&&probes[2].calls==0);
    probes[0].status=RAGE_RESOURCE_READY;
    assert(ResourceProviderResolve(providers,3,&selected)==RAGE_RESOURCE_READY&&selected==0);
    probes[0].status=probes[1].status=probes[2].status=RAGE_RESOURCE_MISSING;
    assert(ResourceProviderResolve(providers,3,&selected)==RAGE_RESOURCE_MISSING&&selected==3);
    assert(ResourceProviderResolve(NULL,0,&selected)==RAGE_RESOURCE_MISSING&&selected==0);
    assert(ResourceProviderResolve(NULL,1,&selected)==RAGE_RESOURCE_ERROR&&selected==1);
    providers[0].resolve=NULL;
    assert(ResourceProviderResolve(providers,3,NULL)==RAGE_RESOURCE_ERROR);
}

int main(void) {
    CheckResourceProviders();
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
