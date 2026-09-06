#ifndef RAGE_MOD_PACKAGE_H
#define RAGE_MOD_PACKAGE_H
#include <stddef.h>

enum { RAGE_MOD_PACKAGE_BYTES = 16384, RAGE_MOD_PACKAGE_DEPENDENCIES = 32 };
typedef struct RageModPackageDependency {
    char packageId[129], version[321];
    int hasVersion;
} RageModPackageDependency;
typedef struct RageModPackage {
    char name[801], region[29], packageId[129];
    char author[481], version[321], description[4801];
    RageModPackageDependency requires[RAGE_MOD_PACKAGE_DEPENDENCIES];
    size_t requirementCount;
    int hasPackageId, hasAuthor, hasVersion, hasDescription;
} RageModPackage;
/* Strict JSON and UTF-8, format 1. Limits count UTF-16 units, matching the
 * existing launcher contract. Output owns decoded strings. Unknown root
 * extensions are ignored; duplicate known fields and unknown dependency
 * fields fail. Failure clears output. No I/O or retained parser allocation. */
int ModPackageParseJSON(const char *bytes, size_t size, RageModPackage *out);
#endif
