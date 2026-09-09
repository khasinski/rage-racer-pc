#include "car_catalog_profile.h"

#include <stdio.h>
#include <string.h>

#include "platform_paths.h"

static const char *const kDefaultProfile = "cars.toml";
static const char *const kJapaneseProfile = "cars.ntscj.toml";

const char *CarCatalogProfileNameForRegion(const char *region) {
    return region != NULL && strcmp(region, "NTSC-J") == 0
               ? kJapaneseProfile : kDefaultProfile;
}

static int Exists(const char *path) {
    FILE *file = path != NULL ? fopen(path, "rb") : NULL;
    if (file == NULL) return 0;
    fclose(file);
    return 1;
}

static int CopyIfMissing(const char *argv0, const char *name) {
    char destination[4096], source[4096];
    FILE *in, *out;
    unsigned char bytes[4096];
    size_t count;
    int ok = 1;

    if (!PlatformUserConfigPath(name, destination, sizeof(destination))) return 0;
    if (Exists(destination)) return 1;
    /* The release-folder catalog is intentionally editable and takes
     * precedence over a profile copied by an earlier release. */
    if (!PlatformFindBundledConfigFile(argv0, name, source, sizeof(source)) &&
        !PlatformFindConfigFile(argv0, name, source, sizeof(source))) return 0;
    if (!PlatformUserConfigDirectory(destination, sizeof(destination)) ||
        !PlatformEnsureDirectory(destination) ||
        !PlatformUserConfigPath(name, destination, sizeof(destination))) return 0;
    in = fopen(source, "rb");
    out = in != NULL ? fopen(destination, "wb") : NULL;
    if (out == NULL) { if (in != NULL) fclose(in); return 0; }
    while ((count = fread(bytes, 1, sizeof(bytes), in)) != 0) {
        if (fwrite(bytes, 1, count, out) != count) { ok = 0; break; }
    }
    if (ferror(in)) ok = 0;
    if (fclose(in) != 0 || fclose(out) != 0) ok = 0;
    return ok;
}

int CarCatalogPrepareProfile(const char *argv0, const char *region,
                             char *path, size_t pathSize) {
    const char *selected = CarCatalogProfileNameForRegion(region);
    if (path == NULL || pathSize == 0) return 0;
    path[0] = '\0';
    if (!CopyIfMissing(argv0, kDefaultProfile) ||
        !CopyIfMissing(argv0, kJapaneseProfile) ||
        !PlatformUserConfigPath(selected, path, pathSize)) {
        path[0] = '\0';
        return 0;
    }
    return 1;
}
