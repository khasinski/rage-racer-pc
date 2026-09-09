#ifndef RAGE_CAR_CATALOG_H
#define RAGE_CAR_CATALOG_H

#include <stddef.h>

#include "game/car.h"

struct CarModelAsset;

enum { RAGE_CAR_CATALOG_ENTRY_COUNT = CAR_MODEL_VARIANT_COUNT,
       RAGE_CAR_CATALOG_TEXT_CAPACITY = 32 };

typedef struct RageCarCatalogEntry {
    int modelIndex, grade, price, upgradePrice, unlockClass, manualOnly;
    char id[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    char name[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    char manufacturer[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    char className[RAGE_CAR_CATALOG_TEXT_CAPACITY];
    GameCarSpec specification;
} RageCarCatalogEntry;

typedef struct RageCarCatalog {
    RageCarCatalogEntry entries[RAGE_CAR_CATALOG_ENTRY_COUNT];
    size_t count;
} RageCarCatalog;

/* Parses a complete flat TOML catalog without changing global game state. */
int CarCatalogParse(const char *text, RageCarCatalog *catalog,
                    char *error, size_t errorSize);
int CarCatalogValidate(const RageCarCatalog *catalog, char *error,
                       size_t errorSize);
/* Loads and validates a profile atomically.  These are called before the
 * title menu, so no menu state can observe a partial catalog. */
int CarCatalogLoadFile(const char *path, char *error, size_t errorSize);
void CarCatalogApplyMetadata(void);
void CarCatalogApplySpecification(int modelIndex, int grade,
                                  GameCarSpec *specification);
void CarCatalogApplyModelAvailability(int modelIndex, int grade,
                                      struct CarModelAsset *asset);
int CarCatalogUnlockClass(int modelIndex, int grade, int fallback);

#endif
