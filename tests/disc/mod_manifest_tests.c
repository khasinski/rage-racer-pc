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
        "\"track.big1.terrain.material.3.variant.1\" = \"hd/tunnel.png\"\n"
        "[materials]\n"
        "\"track.big1.terrain.material.3\" = "
        "\"unlit blend 0.2 0 1 1 1 1 0.4 0.3 0.2\"\n";
    static const char traversal[] =
        "[textures]\n\"track.big1.terrain.material.3\" = \"../secret.png\"\n";
    static const char invalidKey[] =
        "[textures]\n\"asset_088.material.3\" = \"texture.png\"\n";
    static const char invalidMaterial[] =
        "[materials]\n\"track.big1.terrain.material.3\" = "
        "\"glow blend 0.2 0 1 1 1 1 0 0 0\"\n";
    RageModManifest manifest;
    const char *path;

    EXPECT(ModManifestParse(valid, sizeof(valid) - 1, &manifest));
    EXPECT(strcmp(manifest.id, "example-hd") == 0);
    EXPECT(manifest.textureCount == 2);
    path = ModManifestFindTexture(
        &manifest, "track.big1.terrain.material.3.variant.1");
    EXPECT(path != NULL && strcmp(path, "hd/tunnel.png") == 0);
    EXPECT(ModManifestFindTexture(&manifest, "missing") == NULL);
    EXPECT(manifest.materialCount == 1);
    EXPECT(strcmp(ModManifestFindMaterialProperties(
                      &manifest, "track.big1.terrain.material.3"),
                  "unlit blend 0.2 0 1 1 1 1 0.4 0.3 0.2") == 0);
    EXPECT(!ModManifestParse(traversal, sizeof(traversal) - 1,
                                 &manifest));
    EXPECT(manifest.errorLine == 2);
    EXPECT(!ModManifestParse(invalidKey, sizeof(invalidKey) - 1,
                                 &manifest));
    EXPECT(!ModManifestParse(invalidMaterial,
                                 sizeof(invalidMaterial) - 1, &manifest));

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
