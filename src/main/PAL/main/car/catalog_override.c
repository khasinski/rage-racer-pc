#include "game/car_catalog.h"
#include <string.h>

const RageCarCatalogEntry *FindCarEntry(const RageCarCatalog *catalog,
                                       int model, int grade) {
    if (catalog == NULL || catalog->count > RAGE_CAR_CATALOG_ENTRY_COUNT ||
        CarCatalogVariant(model, grade) < 0) return NULL;
    for (size_t i = 0; i < catalog->count; i++) {
        const RageCarCatalogEntry *entry = &catalog->entries[i];
        if (entry->modelIndex == model && entry->grade == grade) return entry;
    }
    return NULL;
}

void ApplyCarSpec(const RageCarCatalogEntry *entry, GameCarSpec *specification) {
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

