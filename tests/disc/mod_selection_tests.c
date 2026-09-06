#include "render/mod_selection.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    RageModSelectionOrder order;
    RageModDependency deps[] = {{RAGE_MOD_PACKAGE_ID,"base","1"},
        {RAGE_MOD_MANIFEST_ID,"addon-mesh",NULL}};
    RageModSelectionEntry entries[] = {
        {"addon","addon-mesh","1","PAL",deps,1},
        {"base","base-mesh","2","PAL",NULL,0},
        {"base","base-mesh","1","PAL",NULL,0}};
    assert(ModSelectionBuildOrder(entries,3,&order));
    assert(order.count == 3 && order.indices[0] == 2 && order.indices[1] == 0 && order.indices[2] == 1);
    deps[0].version = NULL;
    assert(!ModSelectionBuildOrder(entries,3,&order));
    assert(order.code == RAGE_MOD_SELECTION_AMBIGUOUS && order.count == 0);
    for (size_t i=0;i<RAGE_MOD_SELECTION_LIMIT;++i) assert(order.indices[i] == 0);
    deps[0].version = "1";
    entries[2].region = "NTSC-U";
    assert(!ModSelectionBuildOrder(entries,3,&order));
    assert(order.code == RAGE_MOD_SELECTION_MISSING && order.modIndex == 0);
    entries[2].region = "PAL";
    entries[2].dependencies = &deps[1]; entries[2].dependencyCount = 1;
    assert(!ModSelectionBuildOrder(entries,3,&order));
    assert(order.code == RAGE_MOD_SELECTION_CYCLE && order.modIndex == 2 && order.dependencyIndex == 0);
    entries[2].dependencyCount = 0;
    deps[0].identity = RAGE_MOD_MANIFEST_ID;
    assert(!ModSelectionBuildOrder(entries,3,&order)); /* Package ID is not a manifest ID. */
    assert(order.code == RAGE_MOD_SELECTION_MISSING);
    deps[0].identity = (RageModIdentity)99;
    assert(!ModSelectionBuildOrder(entries,3,&order));
    assert(order.code == RAGE_MOD_SELECTION_INVALID);
    assert(ModSelectionBuildOrder(NULL,0,&order) && order.count == 0 && !order.error);
    assert(!ModSelectionBuildOrder(NULL,1,&order));
    assert(!ModSelectionBuildOrder(entries,3,NULL));
    assert(!ModSelectionBuildOrder(entries,RAGE_MOD_SELECTION_LIMIT+1,&order));
    RageModSelectionEntry chain[RAGE_MOD_SELECTION_LIMIT] = {0};
    RageModDependency links[RAGE_MOD_SELECTION_LIMIT] = {0};
    char ids[RAGE_MOD_SELECTION_LIMIT][16];
    for (size_t i=0;i<RAGE_MOD_SELECTION_LIMIT;++i) {
        snprintf(ids[i],sizeof(ids[i]),"package-%zu",i);
        chain[i].packageId = ids[i]; chain[i].region = "PAL";
        if (i+1 < RAGE_MOD_SELECTION_LIMIT) {
            links[i].id = ids[i+1]; chain[i].dependencies = &links[i]; chain[i].dependencyCount = 1;
        }
    }
    assert(ModSelectionBuildOrder(chain,RAGE_MOD_SELECTION_LIMIT,&order));
    for (size_t i=0;i<RAGE_MOD_SELECTION_LIMIT;++i)
        assert(order.indices[i] == RAGE_MOD_SELECTION_LIMIT-1-i);
    chain[0].dependencyCount = RAGE_MOD_DEPENDENCY_LIMIT+1;
    assert(!ModSelectionBuildOrder(chain,RAGE_MOD_SELECTION_LIMIT,&order));
    assert(order.code == RAGE_MOD_SELECTION_INVALID && order.count == 0);
    puts("shared package/manifest graph, versions, regions, cycles and bounds passed");
    return 0;
}
