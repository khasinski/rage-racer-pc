#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mod_assets.h"

static size_t s_room;
static int failures;

#define EXPECT(value) do { if (!(value)) { failures++;                         \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__,  \
            #value);                                                           \
} } while (0)

/* The real one derives the environment name from the key; mods.directory has
 * no alias, so RAGE_PORT_MODS_DIRECTORY is what it looks for. */
const char *RuntimeConfigGet(const char *key) {
    char name[192];
    size_t at = sizeof("RAGE_PORT_") - 1;
    memcpy(name, "RAGE_PORT_", at);
    for (; *key != '\0' && at + 1 < sizeof(name); key++)
        name[at++] = *key == '.' ? '_' : (char)toupper((unsigned char)*key);
    name[at] = '\0';
    return getenv(name);
}

size_t PortAssetRoomAt(const void *at) {
    (void)at;
    return s_room;
}

int TexturePatchAsset(const char *directory, int assetIndex,
                          unsigned char *data, size_t size) {
    (void)directory;
    (void)assetIndex;
    (void)data;
    (void)size;
    return 0;
}

static int WriteFile(const char *path, const unsigned char *data, size_t size) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) return 0;
    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

int main(void) {
    char root[] = "/tmp/rage-mod-fallback-XXXXXX";
    char raw[512], asset0[1024], asset1[1024];
    unsigned char destination[16];
    static const unsigned char valid[] = {1, 2, 3, 4, 5, 6};
    static const unsigned char oversized[32] = {9};

    EXPECT(mkdtemp(root) != NULL);
    snprintf(raw, sizeof(raw), "%s/raw", root);
    EXPECT(mkdir(raw, 0700) == 0);
    snprintf(asset0, sizeof(asset0), "%s/asset_000.bin", raw);
    snprintf(asset1, sizeof(asset1), "%s/asset_001.bin", raw);
    EXPECT(WriteFile(asset0, valid, sizeof(valid)));
    EXPECT(WriteFile(asset1, oversized, sizeof(oversized)));
    EXPECT(setenv("RAGE_PORT_MODS_DIRECTORY", root, 1) == 0);

    memset(destination, 0xA5, sizeof(destination));
    s_room = sizeof(destination);
    EXPECT(ModAssetLoad(0, destination, 2) == 4);
    EXPECT(memcmp(destination, valid, sizeof(valid)) == 0);

    memset(destination, 0xA5, sizeof(destination));
    s_room = 8;
    EXPECT(ModAssetLoad(1, destination, 2) == 0);
    EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);

    s_room = 0;
    EXPECT(ModAssetLoad(0, destination, 2) == 0);
    EXPECT(destination[0] == 0xA5 && destination[15] == 0xA5);

    EXPECT(ModAssetLoad(2, destination, 2) == 0);
    unlink(asset0);
    unlink(asset1);
    rmdir(raw);
    rmdir(root);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
