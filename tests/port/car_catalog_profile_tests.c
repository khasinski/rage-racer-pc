#include <stdio.h>
#include <string.h>

#include "car_catalog_profile.h"

static int failures;

static void Expect(const char *region, const char *expected) {
    const char *actual = CarCatalogProfileNameForRegion(region);
    if (strcmp(actual, expected) == 0) return;
    fprintf(stderr, "profile for %s: expected %s, got %s\n",
            region != NULL ? region : "(null)", expected, actual);
    failures++;
}

int main(void) {
    Expect("PAL", "cars.toml");
    Expect("NTSC-U", "cars.toml");
    Expect("NTSC-J", "cars.ntscj.toml");
    Expect("unknown", "cars.toml");
    Expect(NULL, "cars.toml");
    if (failures != 0) return 1;
    puts("car catalog profile selection passed");
    return 0;
}
