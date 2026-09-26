#ifndef RAGE_CAR_CATALOG_H
#define RAGE_CAR_CATALOG_H
#include "game/car_catalog.h"

struct CarModelAsset;

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
