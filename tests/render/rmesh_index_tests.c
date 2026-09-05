#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh_index.h"
#include "render/asset_path.h"

static int failures;
#define EXPECT(value) do { if (!(value)) { failures++; \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__, #value); \
} } while (0)

int main(void) {
    {
        const char valid[] = "# environment\r\n 96 512 128 sky/a.rgba\r\n97 512 256 sky/b.rgba";
        RageRuntimeEnvironmentLocation location;
        size_t line = 99;
        EXPECT(EnvironmentIndexValidate(valid, sizeof(valid) - 1, 16384, &line));
        EXPECT(line == 0);
        EXPECT(EnvironmentIndexFind(valid, sizeof(valid) - 1, 97, 16384, &location));
        EXPECT(location.width == 512 && location.height == 256);
        EXPECT(location.pathLength == 10 && memcmp(location.path, "sky/b.rgba", 10) == 0);
        const char *bad[] = {
            "96 0 128 a", "96 512 16385 a", "96 -1 128 a", "4294967296 1 1 a",
            "96 4294967296 1 a", "96 1 1 ../a", "96 1 1 /a", "96 1 1 a extra",
            "96 1 1", "96 1 1 a\n96 2 2 b", "96 1 1 C:\\a"
        };
        for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
            EXPECT(!EnvironmentIndexValidate(bad[i], strlen(bad[i]), 16384, &line));
            EXPECT(line == (i == 9 ? 2u : 1u));
        }
        const char nul[] = "# hidden\0value\n96 1 1 a";
        EXPECT(!EnvironmentIndexValidate(nul, sizeof(nul) - 1, 16384, &line));
        EXPECT(line == 1);
        EXPECT(!EnvironmentIndexFind(valid, sizeof(valid) - 1, 98, 16384, &location));
        EXPECT(location.path == NULL && location.width == 0 && location.height == 0);
        EXPECT(!EnvironmentIndexFind(NULL, 10, 96, 16384, &location));
        EXPECT(!EnvironmentIndexFind(valid, sizeof(valid) - 1, 96, 16384, NULL));
        EXPECT(EnvironmentIndexValidate("", 0, 16384, &line));
        EXPECT(!EnvironmentIndexValidate(NULL, 0, 16384, &line));
        EXPECT(!EnvironmentIndexValidate(valid, sizeof(valid) - 1, 0, &line));
        /* Exercise growth beyond 16 entries and sorting independently of input
         * order. The earliest duplicate row must win, not the smallest key. */
        char many[8192];
        size_t used = 0;
        for (unsigned i = 200; i > 0; --i)
            used += (size_t)snprintf(many + used, sizeof(many) - used,
                                    "%u 1 1 sky/a.rgba\r", i);
        EXPECT(EnvironmentIndexValidate(many, used, 16384, &line));
        EXPECT(line == 0);
        EXPECT(EnvironmentIndexFind(many, used, 1, 16384, &location));
        EXPECT(location.width == 1 && location.height == 1);
        used += (size_t)snprintf(many + used, sizeof(many) - used,
                                "200 1 1 b\r\n1 1 1 c");
        EXPECT(!EnvironmentIndexValidate(many, used, 16384, &line));
        EXPECT(line == 201);
        const char boundary[] = "0 1 1 a\n4294967295 16384 16384 b";
        EXPECT(EnvironmentIndexValidate(boundary, sizeof(boundary) - 1, 16384, NULL));
        EXPECT(EnvironmentIndexFind(boundary, sizeof(boundary) - 1,
                                    UINT32_MAX, 16384, &location));
        EXPECT(location.width == 16384 && location.height == 16384);
        EXPECT(location.path == boundary + sizeof(boundary) - 2);
        EXPECT(location.pathLength == 1);
        /* Counted input must not consume malformed bytes outside its extent. */
        const char counted[] = "7 1 1 a\ninvalid";
        EXPECT(EnvironmentIndexValidate(counted, 8, 16384, NULL));
        EXPECT(!EnvironmentIndexValidate(counted, sizeof(counted) - 1, 16384, &line));
        EXPECT(line == 2);
    }
    static const char index[] =
        "# rage-rmesh-index v2\n"
        "10 model models/car.rmesh models/car.rmat\n"
        "91 track-model-1 models/track-bank-1.rmesh models/track-bank-1.rmat\n"
        "91 track-model-2 models/track-bank-2.rmesh models/track-bank-2.rmat\n"
        "91 course models/track-course.rmesh models/track-course.rmat\n"
        "91 terrain models/track-terrain.rmesh models/track-terrain.rmat\n";
    RageRuntimeAssetLocation asset;
    size_t errorLine = 999;
    EXPECT(RuntimeIndexValidate(index, sizeof(index) - 1, &errorLine));
    EXPECT(errorLine == 0);
    {
        const char *badRows[] = {
            "1 model a b\n1 model c d", "1 unknown a b", "1 model a",
            "1 model a b extra", "1 model ../a b", "4294967296 model a b"
        };
        char document[256];
        for (size_t i = 0; i < sizeof(badRows) / sizeof(badRows[0]); ++i) {
            snprintf(document, sizeof(document), "# rage-rmesh-index v2\r\n\r\n%s", badRows[i]);
            EXPECT(!RuntimeIndexValidate(document, strlen(document), &errorLine));
            EXPECT(errorLine == (i == 0 ? 4u : 3u));
        }
        const char allowed[] = "# rage-rmesh-index v2\n  # comment\n"
                               "1 model a b\n1 terrain c d\n";
        EXPECT(RuntimeIndexValidate(allowed, sizeof(allowed) - 1, NULL));
        const char hidden[] = "# rage-rmesh-index v2\n# comment\0hidden\n";
        EXPECT(!RuntimeIndexValidate(hidden, sizeof(hidden) - 1, &errorLine));
        EXPECT(errorLine == 2);
        EXPECT(!RuntimeIndexValidate(NULL, 0, &errorLine));
        EXPECT(errorLine == 1);
        /* Exercise scratch-array growth and sorting with descending keys. */
        char many[8192];
        size_t used = (size_t)snprintf(many, sizeof(many), "# rage-rmesh-index v2\n");
        for (unsigned i = 200; i > 0; --i)
            used += (size_t)snprintf(many + used, sizeof(many) - used, "%u model a b\n", i);
        EXPECT(RuntimeIndexValidate(many, used, &errorLine));
        EXPECT(errorLine == 0);
        used += (size_t)snprintf(many + used, sizeof(many) - used, "100 model a b\n");
        EXPECT(!RuntimeIndexValidate(many, used, &errorLine));
        EXPECT(errorLine == 202);
    }

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
    {
        const char *bad[] = {"../mesh", "/mesh", "a/../mesh", "a/./mesh",
                             "a//mesh", "a/", "C:mesh", "a\\mesh", ".", ".."};
        char record[256];
        for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
            EXPECT(!AssetPathIsRelativeFile(bad[i], strlen(bad[i])));
            for (int material = 0; material < 2; ++material) {
                snprintf(record, sizeof(record), "10 model %s %s\n",
                         material ? "good.rmesh" : bad[i],
                         material ? bad[i] : "good.rmat");
                memset(&asset, 0x7f, sizeof(asset));
                EXPECT(!RuntimeIndexFind(record, strlen(record), 10,
                                        RAGE_RENDER_ASSET_MODEL_BANK, &asset));
                EXPECT(asset.meshPath == NULL && asset.materialPath == NULL);
            }
        }
        const char extra[] = "10 model good.rmesh good.rmat extra\n";
        EXPECT(!RuntimeIndexFind(extra, sizeof(extra) - 1, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK, &asset));
        const char nul[] = "10 model good\0bad good.rmat\n";
        EXPECT(!RuntimeIndexFind(nul, sizeof(nul) - 1, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK, &asset));
        const char counted[] = {'a', '/', 'b'};
        EXPECT(AssetPathIsRelativeFile(counted, sizeof(counted)));
        EXPECT(AssetPathIsRelativeFile("a file/b.rmesh", 14));
        EXPECT(!AssetPathIsRelativeFile(NULL, 0));
        EXPECT(!AssetPathIsRelativeFile("", 0));
        EXPECT(!AssetPathIsRelativeFile("a\tb", 3));
        EXPECT(!AssetPathIsRelativeFile("a\x7f" "b", 3));
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
