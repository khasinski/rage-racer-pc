#ifndef RAGE_CAR_CATALOG_H
#define RAGE_CAR_CATALOG_H

#include <stddef.h>
#include <stdint.h>

#include "game/car.h"

struct CarModelAsset;

enum { RAGE_CAR_CATALOG_ENTRY_COUNT = CAR_MODEL_VARIANT_COUNT,
       RAGE_CAR_CATALOG_TEXT_CAPACITY = 32 };

/* A catalog is an overlay: fields absent from a record retain rage.bin data. */
typedef enum RageCarCatalogField {
    RAGE_CAR_FIELD_ID = 1ull << 0, RAGE_CAR_FIELD_MODEL = 1ull << 1,
    RAGE_CAR_FIELD_GRADE = 1ull << 2, RAGE_CAR_FIELD_NAME = 1ull << 3,
    RAGE_CAR_FIELD_MANUFACTURER = 1ull << 4, RAGE_CAR_FIELD_CLASS = 1ull << 5,
    RAGE_CAR_FIELD_PRICE = 1ull << 6, RAGE_CAR_FIELD_UPGRADE_PRICE = 1ull << 7,
    RAGE_CAR_FIELD_UNLOCK_CLASS = 1ull << 8, RAGE_CAR_FIELD_MANUAL_ONLY = 1ull << 9,
    RAGE_CAR_FIELD_TORQUE_CURVE = 1ull << 10, RAGE_CAR_FIELD_TORQUE_BAND = 1ull << 11,
    RAGE_CAR_FIELD_TORQUE_LOSS_VALUE = 1ull << 12, RAGE_CAR_FIELD_TORQUE_LOSS_RPM = 1ull << 13,
    RAGE_CAR_FIELD_GEAR_LOAD = 1ull << 14, RAGE_CAR_FIELD_GEAR_RATIO = 1ull << 15,
    RAGE_CAR_FIELD_TORQUE_SCALE = 1ull << 16, RAGE_CAR_FIELD_SHIFT_POINTS = 1ull << 17,
    RAGE_CAR_FIELD_REV_LIMIT = 1ull << 18, RAGE_CAR_FIELD_AUTOMATIC_ACCELERATION_SCALE = 1ull << 19,
    RAGE_CAR_FIELD_TOP_GEAR = 1ull << 20, RAGE_CAR_FIELD_REDLINE = 1ull << 21,
    RAGE_CAR_FIELD_STEERING_GRIP_RESPONSE = 1ull << 22, RAGE_CAR_FIELD_STEER_RESPONSE = 1ull << 23,
    RAGE_CAR_FIELD_REFERENCE_TURN_RADIUS = 1ull << 24,
    RAGE_CAR_FIELD_NEGCON_STEERING_ASSIST_SCALE = 1ull << 25,
    RAGE_CAR_FIELD_SPEED_DRAG_DIVISOR = 1ull << 26,
    RAGE_CAR_FIELD_BASE_STEERING_GRIP = 1ull << 27
} RageCarCatalogField;

typedef struct RageCarCatalogEntry {
    int modelIndex, grade, price, upgradePrice, unlockClass, manualOnly;
    char id[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    char name[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    char manufacturer[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    char className[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    uint64_t fields;
    GameCarSpec specification;
} RageCarCatalogEntry;

typedef struct RageCarCatalog {
    RageCarCatalogEntry entries[RAGE_CAR_CATALOG_ENTRY_COUNT];
    size_t count;
} RageCarCatalog;

/* Parses a flat TOML overlay without changing global game state. */
int CarCatalogParse(const char *text, RageCarCatalog *catalog,
                    char *error, size_t errorSize);
int CarCatalogValidate(const RageCarCatalog *catalog, char *error,
                       size_t errorSize);
/* Loads and validates a profile atomically.  These are called before the
 * title menu, so no menu state can observe a partial catalog. */
int CarCatalogLoadFile(const char *path, char *error, size_t errorSize);
void CarCatalogClearOverrides(void);
void CarCatalogApplyMetadata(void);
void CarCatalogApplySpecification(int modelIndex, int grade,
                                  GameCarSpec *specification);
void CarCatalogApplyModelAvailability(int modelIndex, int grade,
                                      struct CarModelAsset *asset);
int CarCatalogUnlockClass(int modelIndex, int grade, int fallback);

#endif
