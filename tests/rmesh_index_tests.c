#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh_index.h"

static int failures;
#define EXPECT(value) do { if (!(value)) { failures++; \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__, #value); \
} } while (0)

int main(void) {
    static const char index[] =
        "# rage-rmesh-index v1\n"
        "10 model models/car.rmesh models/car.rmat\n"
        "91 course models/track-course.rmesh models/track-course.rmat\n"
        "91 terrain models/track-terrain.rmesh models/track-terrain.rmat\n";
    RageRuntimeAssetLocation asset;

    EXPECT(RageRuntimeIndexFind(index, sizeof(index) - 1, 91,
                                RAGE_RENDER_ASSET_TERRAIN, &asset));
    EXPECT(asset.meshPathLength == strlen("models/track-terrain.rmesh"));
    EXPECT(memcmp(asset.meshPath, "models/track-terrain.rmesh",
                  asset.meshPathLength) == 0);
    EXPECT(RageRuntimeIndexFind(index, sizeof(index) - 1, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK, &asset));
    EXPECT(!RageRuntimeIndexFind(index, sizeof(index) - 1, 10,
                                 RAGE_RENDER_ASSET_COURSE, &asset));
    EXPECT(!RageRuntimeIndexFind("999999999999 model x y\n", 24, 1,
                                 RAGE_RENDER_ASSET_MODEL_BANK, &asset));
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

