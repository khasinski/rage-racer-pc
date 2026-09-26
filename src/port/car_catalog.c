#include "car_catalog.h"

#include <string.h>

#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"

static RageCarCatalog s_Catalog;
static int s_CatalogLoaded;

int CarCatalogLoadFile(const char *path, char *error, size_t errorSize) {
    if (!LoadCarCatalog(path, &s_Catalog, error, errorSize)) return 0;
    s_CatalogLoaded = 1;
    return 1;
}

void CarCatalogClearOverrides(void) {
    memset(&s_Catalog, 0, sizeof(s_Catalog));
    s_CatalogLoaded = 0;
}

static const RageCarCatalogEntry *FindEntry(int modelIndex, int grade) {
    return s_CatalogLoaded ? FindCarEntry(&s_Catalog, modelIndex, grade) : NULL;
}

void CarCatalogApplyMetadata(void) {
    size_t index;
    if (!s_CatalogLoaded) return;
    for (index = 0; index < s_Catalog.count; index++) {
        const RageCarCatalogEntry *entry = &s_Catalog.entries[index];
        const int variant = CarCatalogVariant(entry->modelIndex, entry->grade);
        if (entry->fields & RAGE_CAR_FIELD_PRICE) g_CarPriceTable[variant] = entry->price;
        if ((entry->fields & RAGE_CAR_FIELD_UPGRADE_PRICE) &&
            variant < CAR_MODEL_VARIANT_COUNT - 1) g_CarTuneUpPriceTable[variant] = entry->upgradePrice;
        if (entry->grade == 0) {
            if (entry->fields & RAGE_CAR_FIELD_NAME) g_NativeCarNames[entry->modelIndex] = entry->name;
            if (entry->fields & RAGE_CAR_FIELD_MANUFACTURER)
                SetCarMaker(entry->modelIndex, entry->manufacturer);
            if (entry->fields & RAGE_CAR_FIELD_CLASS) g_NativeCarClassNames[entry->modelIndex] = entry->className;
        }
    }
}

void CarCatalogApplySpecification(int modelIndex, int grade, GameCarSpec *specification) {
    ApplyCarSpec(FindEntry(modelIndex, grade), specification);
}

void CarCatalogApplyModelAvailability(int modelIndex, int grade, struct CarModelAsset *asset) {
    const RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    if (entry == NULL) return;
    if (!(entry->fields & RAGE_CAR_FIELD_MANUAL_ONLY)) return;
    if (asset != NULL) asset->transmissionAvailable = (u8)!entry->manualOnly;
    /* A pre-existing modded save can request automatic even though the menu
     * correctly hides that row.  Keep the runtime state consistent with the
     * catalog when this model is loaded. */
    if (entry->manualOnly && g_CarTable != NULL &&
        (unsigned)modelIndex < GAME_CAR_COUNT) {
        g_CarTable[modelIndex].transmission = 1;
    }
}

int CarCatalogUnlockClass(int modelIndex, int grade, int fallback) {
    const RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    return entry != NULL && (entry->fields & RAGE_CAR_FIELD_UNLOCK_CLASS)
               ? entry->unlockClass : fallback;
}
