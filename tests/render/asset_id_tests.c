#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/asset_id.h"

static int failures;

#define EXPECT_STRING(expected, actual) do {                                  \
    if (strcmp((expected), (actual)) != 0) {                                   \
        fprintf(stderr, "%s:%d: expected %s, got %s\n", __FILE__, __LINE__, \
                (expected), (actual));                                         \
        failures++;                                                            \
    }                                                                          \
} while (0)

#define EXPECT(value) do { if (!(value)) { failures++;                         \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__,  \
            #value);                                                           \
} } while (0)

int main(void) {
    char id[128];

    EXPECT(AssetMaterialId(id, sizeof(id), 0x58,
                               RAGE_RENDER_ASSET_TERRAIN, 3));
    EXPECT_STRING("track.big1.terrain.material.3", id);
    EXPECT(AssetMaterialVariantId(id, sizeof(id), 0x5e,
                                      RAGE_RENDER_ASSET_COURSE, 12, 1));
    EXPECT_STRING("track.oval1.course.material.12.variant.1", id);
    EXPECT(AssetMaterialId(id, sizeof(id), 0x80,
                               RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 4));
    EXPECT_STRING("track.big6.model-bank-1.material.4", id);
    EXPECT(AssetMaterialId(id, sizeof(id), 16,
                               RAGE_RENDER_ASSET_MODEL_BANK, 7));
    EXPECT_STRING("car.3.material.7", id);
    strcpy(id, "stale");
    EXPECT(!AssetMaterialId(id, sizeof(id), 87,
                                RAGE_RENDER_ASSET_TERRAIN, 0));
    EXPECT_STRING("", id);
    strcpy(id, "stale");
    EXPECT(!AssetMaterialId(id, 8, 0x58,
                                RAGE_RENDER_ASSET_TERRAIN, 0));
    EXPECT_STRING("", id);
    EXPECT(!AssetMaterialVariantId(NULL, sizeof(id), 0x58,
                                   RAGE_RENDER_ASSET_TERRAIN, 0, 1));
    EXPECT(!AssetMaterialVariantId(id, 0, 0x58,
                                   RAGE_RENDER_ASSET_TERRAIN, 0, 1));

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
