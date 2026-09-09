#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "car_catalog.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"

s32 g_CarPriceTable[CAR_PRICE_COUNT];
s32 g_CarTuneUpPriceTable[CAR_TUNE_UP_PRICE_COUNT];
const char *g_NativeCarNames[GAME_CAR_COUNT];
const char *g_NativeCarClassNames[GAME_CAR_COUNT];
const char *g_NativeCarManufacturerNames[GAME_CAR_COUNT];
static CarEntry carTable[GAME_CAR_COUNT];
CarEntry *g_CarTable = carTable;

static int first[] = {0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31};

static int VerifyShippedManualOnlyVariants(const char *path, char *error) {
    static const int variants[] = {7, 8, 21, 22, 26, 27, 28, 30, 31};
    size_t i;

    if (!CarCatalogLoadFile(path, error, 256)) return 0;
    for (i = 0; i < sizeof(variants) / sizeof(variants[0]); i++) {
        int variant = variants[i], model = 0;
        GameCarSpec specification;
        CarModelAsset asset;
        while (model + 1 < GAME_CAR_COUNT && variant >= first[model + 1]) model++;
        memset(&specification, 0, sizeof(specification));
        CarCatalogApplySpecification(model, variant - first[model], &specification);
        if (specification.automaticAccelerationScale < 900 ||
            specification.automaticAccelerationScale > 1000 ||
            specification.shiftPoints[0].downshiftSpeed <= 0 ||
            specification.shiftPoints[0].upshiftSpeed <=
                specification.shiftPoints[0].downshiftSpeed) {
            fprintf(stderr, "%s variant %d has no usable reconstructed automatic setup\n",
                    path, variant);
            return 0;
        }
        memset(&asset, 0, sizeof(asset));
        g_CarTable[model].transmission = 0;
        CarCatalogApplyModelAvailability(model, variant - first[model], &asset);
        if (asset.transmissionAvailable != 0 || g_CarTable[model].transmission != 1) {
            fprintf(stderr, "%s variant %d is not enforced manual-only\n", path, variant);
            return 0;
        }
    }
    return 1;
}

static int VerifySqualdonAutomaticProfile(const char *path, char *error) {
    GameCarSpec specification;
    if (!CarCatalogLoadFile(path, error, 256)) return 0;
    memset(&specification, 0, sizeof(specification));
    CarCatalogApplySpecification(12, 0, &specification);
    return specification.automaticAccelerationScale == 985 &&
        specification.shiftPoints[0].downshiftSpeed == 520 &&
        specification.shiftPoints[0].upshiftSpeed == 694 &&
        specification.shiftPoints[4].downshiftSpeed == 1408 &&
        specification.shiftPoints[4].upshiftSpeed == 1878;
}

static char *MakeCatalog(int rows, int badSpec) {
    size_t capacity = 65536, used = 0;
    char *text = malloc(capacity);
    int model = 0, variant;
    if (text == NULL) return NULL;
    text[0] = '\0';
    for (variant = 0; variant < rows; variant++) {
        char row[2048];
        while (model + 1 < GAME_CAR_COUNT && variant >= first[model + 1]) model++;
        snprintf(row, sizeof(row),
            "[[cars]]\nid = \"car_%d_%d\"\nmodel = %d\ngrade = %d\n"
            "name = \"NAME%d\"\nmanufacturer = \"MAKE%d\"\nclass = \"CLASS%d\"\n"
            "price = %d\nupgrade_price = %d\nunlock_class = %d\nmanual_only = %s\n"
            "torque_curve = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]\n"
            "torque_band = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]\n"
            "torque_loss_value = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]\n"
            "torque_loss_rpm = [1, 1, 1, 1, 1, 1, 1, 1, 1]\n"
            "gear_load = [1, 1, 1, 1, 1, 1]\ngear_ratio = [1, 2, 3, 4, 5, 6, 7]\n"
            "rev_limit = %d\nautomatic_acceleration_scale = 1000\ntop_gear = 6\nredline = 1\n"
            "steering_grip_response = 1\nsteer_response = 1\nreference_turn_radius = 1\n"
            "negcon_steering_assist_scale = 1\nspeed_drag_divisor = 1\nbase_steering_grip = 1\n"
            "torque_scale = [1, 1, 1, 1, 1, 1]\nshift_points = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]\n",
            model, variant - first[model], model, variant - first[model],
            model, model, model, variant * 100, variant * 100 + 1,
            variant + 1, variant == 7 ? "true" : "false", badSpec && variant == 0 ? 0 : 9000);
        if (used + strlen(row) + 1 >= capacity) { free(text); return NULL; }
        strcpy(text + used, row); used += strlen(row);
    }
    return text;
}

int main(void) {
    char error[256]; RageCarCatalog catalog; char *valid = MakeCatalog(32, 0);
    char *shortCatalog = MakeCatalog(31, 0); char *bad = MakeCatalog(32, 1);
    CarModelAsset asset;
    if (valid == NULL || shortCatalog == NULL || bad == NULL ||
        !CarCatalogParse(valid, &catalog, error, sizeof(error)) ||
        !CarCatalogParse(shortCatalog, &catalog, error, sizeof(error)) ||
        CarCatalogParse(bad, &catalog, error, sizeof(error))) {
        fprintf(stderr, "catalog parse/validation failed: %s\n", error); return 1;
    }
    { FILE *file = fopen("car_catalog_test.toml", "wb");
      if (file == NULL || fwrite(valid, 1, strlen(valid), file) != strlen(valid) || fclose(file) != 0 ||
          !CarCatalogLoadFile("car_catalog_test.toml", error, sizeof(error))) return 1; }
    remove("car_catalog_test.toml");
    CarCatalogApplyMetadata();
    if (g_CarPriceTable[31] != 3100 || strcmp(g_NativeCarNames[12], "NAME12") != 0 ||
        CarCatalogUnlockClass(2, 0, 0) != 8) return 1;
    memset(&catalog.entries[0].specification, 0, sizeof(GameCarSpec));
    catalog.entries[0].specification.revLimit = 1234;
    /* The loaded catalog, rather than the local parse buffer, drives race data. */
    { GameCarSpec spec; memset(&spec, 0, sizeof(spec)); CarCatalogApplySpecification(0, 0, &spec);
      if (spec.revLimit != 9000) return 1; }
    memset(&asset, 0, sizeof(asset)); asset.transmissionAvailable = 1;
    g_CarTable[2].transmission = 0;
    CarCatalogApplyModelAvailability(2, 0, &asset);
    if (asset.transmissionAvailable != 0 || g_CarTable[2].transmission != 1) return 1;
    /* A sparse catalog overlays the retail spec decoded from rage.bin. */
    { const char *sparse =
          "[[cars]]\n"
          "id = \"test\"\nmodel = 2\ngrade = 0\n"
          "price = 777\nmanual_only = false\n"
          "automatic_acceleration_scale = 985\n"
          "shift_points = [520, 694, 682, 910, 918, 1224, 1143, 1525, 1408, 1878, 1899, 2532]\n";
      FILE *file = fopen("car_catalog_test.toml", "wb");
      GameCarSpec retail, applied;
      if (file == NULL || fwrite(sparse, 1, strlen(sparse), file) != strlen(sparse) ||
          fclose(file) != 0 || !CarCatalogLoadFile("car_catalog_test.toml", error, sizeof(error))) return 1;
      memset(&retail, 0x5a, sizeof(retail)); retail.revLimit = 8123;
      retail.automaticAccelerationScale = 1000; retail.gearRatio[1] = 321;
      applied = retail;
      CarCatalogApplySpecification(2, 0, &applied);
      CarCatalogApplyMetadata();
      memset(&asset, 0, sizeof(asset)); asset.transmissionAvailable = 0;
      CarCatalogApplyModelAvailability(2, 0, &asset);
      if (g_CarPriceTable[7] != 777 || applied.revLimit != 8123 ||
          applied.gearRatio[1] != 321 || applied.automaticAccelerationScale != 985 ||
          applied.shiftPoints[0].upshiftSpeed != 694 || asset.transmissionAvailable != 1) return 1;
      remove("car_catalog_test.toml");
    }
    /* A malformed override is rejected atomically. Clearing it leaves the
     * rage.bin defaults untouched, which is the path main() takes at boot. */
    { const char *broken = "[[cars]]\nid = \"bad\"\nmodel = 2\ngrade = 0\nprice = nope\n";
      FILE *file = fopen("car_catalog_test.toml", "wb"); GameCarSpec retail;
      if (file == NULL || fwrite(broken, 1, strlen(broken), file) != strlen(broken) ||
          fclose(file) != 0) return 1;
      CarCatalogClearOverrides();
      if (CarCatalogLoadFile("car_catalog_test.toml", error, sizeof(error))) return 1;
      memset(&retail, 0, sizeof(retail)); retail.revLimit = 7654;
      CarCatalogApplySpecification(2, 0, &retail);
      if (retail.revLimit != 7654) return 1;
      remove("car_catalog_test.toml");
    }
    if (!VerifyShippedManualOnlyVariants(RAGE_SOURCE_DIRECTORY "/cars.toml", error) ||
        !VerifyShippedManualOnlyVariants(RAGE_SOURCE_DIRECTORY "/cars.ntscj.toml", error) ||
        !VerifySqualdonAutomaticProfile(RAGE_SOURCE_DIRECTORY "/cars.toml", error) ||
        !VerifySqualdonAutomaticProfile(RAGE_SOURCE_DIRECTORY "/cars.ntscj.toml", error)) {
        fprintf(stderr, "shipped catalog did not parse: %s\n", error);
        return 1;
    }
    free(valid); free(shortCatalog); free(bad);
    puts("car catalog parse and application passed"); return 0;
}
