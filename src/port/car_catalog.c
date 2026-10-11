#include "car_catalog.h"
#include <string.h>
#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"

static RageCarCatalog s_Catalog;
static int s_CatalogLoaded;

int CarCatalogLoadFile(const char *path, char *error, size_t errorSize) {
    RageCarCatalog next, ordered;
    size_t index;
    if (!CarCatalogReadFile(path, &next, error, errorSize)) return 0;
    memset(&ordered, 0, sizeof(ordered));
    ordered.count = next.count;
    for (index=0; index<next.count; index++) {
        RageCarCatalogEntry *entry=&next.entries[index];
        ordered.entries[CarCatalogVariant(entry->modelIndex,entry->grade)] = *entry;
    }
    s_Catalog=ordered;s_CatalogLoaded=1;return 1;
}

void CarCatalogClearOverrides(void) {
    memset(&s_Catalog, 0, sizeof(s_Catalog));
    s_CatalogLoaded = 0;
}

static RageCarCatalogEntry *FindEntry(int modelIndex, int grade) {
    int variant;
    if (!s_CatalogLoaded || modelIndex < 0 || modelIndex >= GAME_CAR_COUNT || grade < 0) return NULL;
    variant = CarCatalogVariant(modelIndex, grade);
    if (variant < 0 || variant >= CAR_MODEL_VARIANT_COUNT) return NULL;
    return &s_Catalog.entries[variant];
}

void CarCatalogApplyMetadata(void) {
    size_t index;
    if (!s_CatalogLoaded) return;
    for (index = 0; index < CAR_MODEL_VARIANT_COUNT; index++) {
        RageCarCatalogEntry *entry = &s_Catalog.entries[index];
        if (entry->fields & RAGE_CAR_FIELD_PRICE) g_CarPriceTable[index] = entry->price;
        if ((entry->fields & RAGE_CAR_FIELD_UPGRADE_PRICE) &&
            index < CAR_MODEL_VARIANT_COUNT - 1) g_CarTuneUpPriceTable[index] = entry->upgradePrice;
        if (entry->grade == 0) {
            if (entry->fields & RAGE_CAR_FIELD_NAME) g_NativeCarNames[entry->modelIndex] = entry->name;
            if (entry->fields & RAGE_CAR_FIELD_MANUFACTURER)
                SetCarMaker(entry->modelIndex, entry->manufacturer);
            if (entry->fields & RAGE_CAR_FIELD_CLASS) g_NativeCarClassNames[entry->modelIndex] = entry->className;
        }
    }
}

void CarCatalogApplySpecification(int modelIndex, int grade, GameCarSpec *specification) {
    RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    if (entry == NULL || specification == NULL) return;
#define COPY_ARRAY(bit, member) if (entry->fields & bit) memcpy(specification->member, entry->specification.member, sizeof(specification->member))
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_CURVE, torqueCurve);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_BAND, torqueBand.values);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_LOSS_VALUE, torqueLossValue);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_LOSS_RPM, torqueLossRpm);
    COPY_ARRAY(RAGE_CAR_FIELD_GEAR_LOAD, gearLoad);
    COPY_ARRAY(RAGE_CAR_FIELD_GEAR_RATIO, gearRatio);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_SCALE, torqueScale);
    COPY_ARRAY(RAGE_CAR_FIELD_SHIFT_POINTS, shiftPoints);
#undef COPY_ARRAY
#define COPY_SCALAR(bit, member) if (entry->fields & bit) specification->member = entry->specification.member
    COPY_SCALAR(RAGE_CAR_FIELD_REV_LIMIT, revLimit);
    COPY_SCALAR(RAGE_CAR_FIELD_AUTOMATIC_ACCELERATION_SCALE, automaticAccelerationScale);
    COPY_SCALAR(RAGE_CAR_FIELD_TOP_GEAR, topGear);
    COPY_SCALAR(RAGE_CAR_FIELD_REDLINE, redline);
    COPY_SCALAR(RAGE_CAR_FIELD_STEERING_GRIP_RESPONSE, steeringGripResponse);
    COPY_SCALAR(RAGE_CAR_FIELD_STEER_RESPONSE, steerResponse);
    COPY_SCALAR(RAGE_CAR_FIELD_REFERENCE_TURN_RADIUS, referenceTurnRadius);
    COPY_SCALAR(RAGE_CAR_FIELD_NEGCON_STEERING_ASSIST_SCALE, negconSteeringAssistScale);
    COPY_SCALAR(RAGE_CAR_FIELD_SPEED_DRAG_DIVISOR, speedDragDivisor);
    COPY_SCALAR(RAGE_CAR_FIELD_BASE_STEERING_GRIP, baseSteeringGrip);
#undef COPY_SCALAR
}

static int s_retailManualOnly;

void CarCatalogSetRetailManualOnly(int retail) { s_retailManualOnly = retail != 0; }

void CarCatalogApplyModelAvailability(int modelIndex, int grade, struct CarModelAsset *asset) {
    RageCarCatalogEntry *entry;
    if (s_retailManualOnly) {
        /* The serialized retail metadata says which bodies are manual-only. */
        if (asset != NULL && asset->transmissionAvailable == 0 &&
            g_CarTable != NULL && (unsigned)modelIndex < GAME_CAR_COUNT)
            g_CarTable[modelIndex].transmission = 1;
        return;
    }
    entry = FindEntry(modelIndex, grade);
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
    RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    return entry != NULL && (entry->fields & RAGE_CAR_FIELD_UNLOCK_CLASS)
               ? entry->unlockClass : fallback;
}
