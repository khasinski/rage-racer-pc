#include "game/car_catalog.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    RageCarCatalog catalog, other;
    char error[128];
    const char *valid = "[[cars]]\nid = \"first\"\nmodel = 0\ngrade = 0\nrev_limit = 9000\n";
    CHECK(CarCatalogParse(valid, &catalog, error, sizeof(error)));
    CHECK(catalog.count == 1 && catalog.entries[0].specification.revLimit == 9000);
    CHECK(CarCatalogParse("[[cars]]\nid = \"other\"\nmodel = 12\ngrade = 0\n", &other, error, sizeof(error)));
    CHECK(strcmp(catalog.entries[0].id, "first") == 0);
    CHECK(FindCarEntry(&catalog, 0, 0) == &catalog.entries[0]);
    CHECK(FindCarEntry(&catalog, 0, 1) == NULL);
    CHECK(FindCarEntry(&other, 12, 0) == &other.entries[0]);
    CHECK(FindCarEntry(&other, 0, 0) == NULL);
    CHECK(FindCarEntry(NULL, 0, 0) == NULL);
    CHECK(FindCarEntry(&catalog, 0, INT_MAX) == NULL);
    RageCarCatalog sparse = {0};
    sparse.count = 2;
    sparse.entries[0] = other.entries[0];
    sparse.entries[1] = catalog.entries[0];
    const RageCarCatalog beforeLookup = sparse;
    CHECK(FindCarEntry(&sparse, 0, 0) == &sparse.entries[1]);
    CHECK(FindCarEntry(&sparse, 12, 0) == &sparse.entries[0]);
    CHECK(FindCarEntry(&sparse, 0, 4) == NULL);
    CHECK(memcmp(&sparse, &beforeLookup, sizeof(sparse)) == 0);
    const RageCarCatalog saved = catalog;

    const char *invalid[] = {
        "", "rev_limit = 1\n", "[[cars]]\nid = \"missing fields\"\n",
        "[[cars]]\nid = \"bad\"\nmodel = 0\ngrade = 4\n",
        "[[cars]]\nid = \"bad\"\nmodel = 0\ngrade = 2147483647\n",
        "[[cars]]\nid = \"bad\"\nmodel = 0\ngrade = 0\nrev_limit = 0\n",
        "[[cars]]\nid = \"bad\"\nmodel = 0\ngrade = 0\nunknown = 1\n",
        "[[cars]]\nid = \"bad\"\nmodel = 0\ngrade = 0\ngrade = 1\n",
        "[[cars]]\nid = \"bad\"\nmodel = 0\ngrade = 0\nshift_points = [0,1]\n",
    };
    const char *badNumbers[] = {
        "price = 99999999999999999999999999999999999999",
        "price = -99999999999999999999999999999999999999",
        "price = +", "price = -", "price = 2147483648",
        "torque_band = [99999999999999999999999999999999999999,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",
        "torque_band = [-99999999999999999999999999999999999999,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",
    };
    for (size_t i = 0; i < sizeof(badNumbers) / sizeof(badNumbers[0]); i++) {
        char text[512];
        snprintf(text, sizeof(text), "%s%s\n", valid, badNumbers[i]);
        CHECK(!CarCatalogParse(text, &catalog, error, sizeof(error)));
        CHECK(memcmp(&catalog, &saved, sizeof(catalog)) == 0);
    }
    CHECK(CarCatalogParse("[[cars]]\nid = \"limits\"\nmodel = 0\ngrade = 0\nprice = 2147483647\ntorque_band = [-2147483648,2147483647,0,0,0,0,0,0,0,0,0,0,0,0,0,0]\n",
                          &other, error, sizeof(error)));
    CHECK(other.entries[0].price == INT32_MAX);
    CHECK(other.entries[0].specification.torqueBand.values[0] == INT32_MIN);
    CHECK(other.entries[0].specification.torqueBand.values[1] == INT32_MAX);
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        CHECK(!CarCatalogParse(invalid[i], &catalog, error, sizeof(error)));
        CHECK(error[0] != '\0' && memcmp(&catalog, &saved, sizeof(catalog)) == 0);
    }
    CHECK(!CarCatalogParse(NULL, &catalog, NULL, 0));
    CHECK(memcmp(&catalog, &saved, sizeof(catalog)) == 0);
    CHECK(!CarCatalogParse(valid, NULL, error, sizeof(error)));
    CHECK(CarCatalogVariant(0, 4) == -1 && CarCatalogVariant(12, 0) == 31);
    CHECK(CarCatalogVariant(-1, 0) == -1 && CarCatalogVariant(0, INT_MAX) == -1);
    other.count = RAGE_CAR_CATALOG_ENTRY_COUNT + 1;
    CHECK(!CarCatalogValidate(&other, error, sizeof(error)));
    CHECK(FindCarEntry(&other, 12, 0) == NULL);
    other.count = 1;
    other.entries[0].grade = INT_MAX;
    CHECK(!CarCatalogValidate(&other, error, sizeof(error)));
    const char *path = "catalog_parse_test.toml";
    FILE *file = fopen(path, "wb");
    CHECK(file != NULL);
    CHECK(fwrite(valid, 1, strlen(valid), file) == strlen(valid));
    CHECK(fclose(file) == 0);
    CHECK(LoadCarCatalog(path, &other, error, sizeof(error)));
    CHECK(memcmp(&other, &saved, sizeof(other)) == 0);
    file = fopen(path, "ab");
    CHECK(file != NULL);
    CHECK(fputc(0, file) == 0);
    CHECK(fputs("invalid trailing data", file) >= 0);
    CHECK(fclose(file) == 0);
    CHECK(!LoadCarCatalog(path, &other, error, sizeof(error)));
    CHECK(memcmp(&other, &saved, sizeof(other)) == 0);
    CHECK(remove(path) == 0);
    CHECK(!LoadCarCatalog(path, &other, error, sizeof(error)));
    CHECK(memcmp(&other, &saved, sizeof(other)) == 0);
    CHECK(!LoadCarCatalog(NULL, &other, NULL, 0));
    CHECK(!LoadCarCatalog(path, NULL, error, sizeof(error)));
    return 0;
}
