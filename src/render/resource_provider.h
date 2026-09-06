#ifndef RAGE_RESOURCE_PROVIDER_H
#define RAGE_RESOURCE_PROVIDER_H
#include <stddef.h>
typedef enum RageResourceStatus {
    RAGE_RESOURCE_MISSING, RAGE_RESOURCE_READY, RAGE_RESOURCE_ERROR
} RageResourceStatus;
typedef struct RageResourceProvider {
    RageResourceStatus (*resolve)(void *context);
    void *context;
} RageResourceProvider;
/* Providers are ordered by explicit precedence. Only MISSING falls through;
 * ERROR is terminal. Callbacks own storage and publish their typed result in
 * context only on READY. The resolver neither allocates nor retains pointers.
 * selected is count on failure/missing, otherwise the successful provider. */
static RageResourceStatus ResourceProviderResolve(const RageResourceProvider *providers,
                                                 size_t count,size_t *selected) {
    if(selected)*selected=count;
    if(!providers&&count)return RAGE_RESOURCE_ERROR;
    for(size_t i=0;i<count;++i) {
        if(!providers[i].resolve)return RAGE_RESOURCE_ERROR;
        RageResourceStatus status=providers[i].resolve(providers[i].context);
        if(status==RAGE_RESOURCE_MISSING)continue;
        if(status!=RAGE_RESOURCE_READY)return RAGE_RESOURCE_ERROR;
        if(selected)*selected=i;
        return RAGE_RESOURCE_READY;
    }
    return RAGE_RESOURCE_MISSING;
}
#endif
