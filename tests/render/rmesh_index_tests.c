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
        "# rage-rmesh-index v2\n"
        "10 model models/car.rmesh models/car.rmat\n"
        "91 track-model-1 models/track-bank-1.rmesh models/track-bank-1.rmat\n"
        "91 track-model-2 models/track-bank-2.rmesh models/track-bank-2.rmat\n"
        "91 course models/track-course.rmesh models/track-course.rmat\n"
        "91 terrain models/track-terrain.rmesh models/track-terrain.rmat\n";
    RageRuntimeAssetLocation asset;

    EXPECT(RuntimeIndexVersion(index, sizeof(index) - 1) == 2);
    EXPECT(RuntimeIndexVersion("# rage-rmesh-index v1\n", 22) == 1);
    EXPECT(RuntimeIndexVersion("10 model x y\n", 13) == 0);
    EXPECT(RuntimeIndexVersion("# rage-rmesh-index vx\n", 22) == 0);
    EXPECT(RuntimeIndexFind(index, sizeof(index) - 1, 91,
                                RAGE_RENDER_ASSET_TERRAIN, &asset));
    EXPECT(asset.meshPathLength == strlen("models/track-terrain.rmesh"));
    EXPECT(memcmp(asset.meshPath, "models/track-terrain.rmesh",
                  asset.meshPathLength) == 0);
    EXPECT(RuntimeIndexFind(index, sizeof(index) - 1, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK, &asset));
    memset(&asset, 0x7f, sizeof(asset));
    EXPECT(!RuntimeIndexFind(index, sizeof(index) - 1, 10,
                                 RAGE_RENDER_ASSET_COURSE, &asset));
    EXPECT(asset.meshPath == NULL && asset.meshPathLength == 0 &&
           asset.materialPath == NULL && asset.materialPathLength == 0);
    EXPECT(RuntimeIndexFind(index, sizeof(index) - 1, 91,
                                RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, &asset));
    EXPECT(asset.meshPathLength == strlen("models/track-bank-1.rmesh"));
    EXPECT(RuntimeIndexFind(index, sizeof(index) - 1, 91,
                                RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2, &asset));
    EXPECT(asset.meshPathLength == strlen("models/track-bank-2.rmesh"));
    EXPECT(!RuntimeIndexFind("999999999999 model x y\n", 24, 1,
                                 RAGE_RENDER_ASSET_MODEL_BANK, &asset));
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
