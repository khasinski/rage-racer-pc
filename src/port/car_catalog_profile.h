#ifndef RAGE_CAR_CATALOG_PROFILE_H
#define RAGE_CAR_CATALOG_PROFILE_H

#include <stddef.h>

/* The Japanese retail revisions share one authored profile. Unknown media uses
 * the international default so a malformed identification cannot select a
 * Japanese-only setup. */
const char *CarCatalogProfileNameForRegion(const char *region);

/* Copies the shipped templates into the user's configuration directory when
 * they do not yet exist, then returns the selected editable profile path. */
int CarCatalogPrepareProfile(const char *argv0, const char *region,
                             char *path, size_t pathSize);

#endif
