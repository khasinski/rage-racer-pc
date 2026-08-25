#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/mod_manifest.h"

static int failures;
#define EXPECT(value) do { if (!(value)) { failures++;                         \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__,  \
            #value);                                                           \
} } while (0)

int main(void) {
    static const char valid[] =
        "# semantic PNG overrides\n"
        "[mod]\n"
        "id=\"example-hd\"\n"
        "\n"
        "[textures]\n"
        "\"track.big1.terrain.material.3\" = \"textures/tunnel.png\"\n"
        "\"track.big1.terrain.material.3.variant.1\" = \"hd/tunnel.png\"\n";
    static const char traversal[] =
        "[textures]\n\"track.big1.terrain.material.3\" = \"../secret.png\"\n";
    static const char invalidKey[] =
        "[textures]\n\"asset_088.material.3\" = \"texture.png\"\n";
    RageModManifest manifest;
    const char *path;

    EXPECT(RageModManifestParse(valid, sizeof(valid) - 1, &manifest));
    EXPECT(strcmp(manifest.id, "example-hd") == 0);
    EXPECT(manifest.textureCount == 2);
    path = RageModManifestFindTexture(
        &manifest, "track.big1.terrain.material.3.variant.1");
    EXPECT(path != NULL && strcmp(path, "hd/tunnel.png") == 0);
    EXPECT(RageModManifestFindTexture(&manifest, "missing") == NULL);
    EXPECT(!RageModManifestParse(traversal, sizeof(traversal) - 1,
                                 &manifest));
    EXPECT(manifest.errorLine == 2);
    EXPECT(!RageModManifestParse(invalidKey, sizeof(invalidKey) - 1,
                                 &manifest));

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
