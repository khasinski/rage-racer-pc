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
    static const char partialThenInvalid[] =
        "[textures]\n"
        "\"track.big1.terrain.material.3\" = \"textures/road.png\"\n"
        "\"track.big1.course.material.2\" = \"../secret.png\"\n";
    static const char invalidKey[] =
        "[textures]\n\"asset_088.material.3\" = \"texture.png\"\n";
    static const char invalidMaterial[] =
        "[materials]\n\"track.big1.terrain.material.3\" = "
        "\"glow blend 0.2 0 1 1 1 1 0 0 0\"\n";
    RageModManifest manifest;
    const char *path;

    EXPECT(ModManifestParse(valid, sizeof(valid) - 1, &manifest));
    EXPECT(manifest.schemaVersion == RAGE_MOD_MANIFEST_SCHEMA_VERSION);
    EXPECT(manifest.error == RAGE_MOD_MANIFEST_OK && manifest.errorLine == 0);
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
    EXPECT(manifest.textureCount == 0 && manifest.materialCount == 0 &&
           manifest.id[0] == '\0');
    EXPECT(!ModManifestParse(partialThenInvalid,
                             sizeof(partialThenInvalid) - 1, &manifest));
    EXPECT(manifest.errorLine == 3 && manifest.textureCount == 0);
    EXPECT(!ModManifestParse(invalidKey, sizeof(invalidKey) - 1,
                                 &manifest));
    EXPECT(!ModManifestParse(invalidMaterial,
                                 sizeof(invalidMaterial) - 1, &manifest));

    {
        static const char versioned[] =
            "[mod]\r\nschema_version = 1 # format, not mod release\r\n"
            "id = \"example-hd\"\r\n[textures]\r\n"
            "\"track.big1.terrain.material.3\" = \"a.png\"";
        static const char *badVersions[] = {
            "0", "2", "4294967295", "-1", "+1", "1.0", "\"1\"",
            "1 garbage", "", "999999999999999999999999999999999999",
            "1\nschema_version=1"
        };
        char input[256];
        EXPECT(ModManifestParse(versioned, sizeof(versioned) - 1, &manifest));
        EXPECT(manifest.schemaVersion == 1 && manifest.textureCount == 1);
        for (size_t i = 0; i < sizeof(badVersions) / sizeof(badVersions[0]); ++i) {
            snprintf(input, sizeof(input), "[mod]\nid=\"old\"\nschema_version=%s",
                     badVersions[i]);
            EXPECT(!ModManifestParse(input, strlen(input), &manifest));
            EXPECT(manifest.schemaVersion == 0 && manifest.id[0] == '\0');
            EXPECT(manifest.textureCount == 0 && manifest.materialCount == 0);
            EXPECT(manifest.errorLine == (i == 10 ? 4u : 3u));
            EXPECT(manifest.error == (i < 3
                ? RAGE_MOD_MANIFEST_UNSUPPORTED_VERSION
                : RAGE_MOD_MANIFEST_INVALID));
        }
        /* Reject a version even when it comes after valid asset declarations. */
        static const char lateVersion[] =
            "[textures]\n\"track.big1\"=\"a.png\"\n[mod]\nschema_version=2";
        EXPECT(!ModManifestParse(lateVersion, sizeof(lateVersion) - 1, &manifest));
        EXPECT(manifest.textureCount == 0 && manifest.errorLine == 4);
        EXPECT(manifest.error == RAGE_MOD_MANIFEST_UNSUPPORTED_VERSION);
        static const char embeddedNull[] = "[mod]\nschema_version=1\0garbage";
        EXPECT(!ModManifestParse(embeddedNull, sizeof(embeddedNull) - 1, &manifest));
        EXPECT(manifest.error == RAGE_MOD_MANIFEST_INVALID && manifest.errorLine == 2);
        static const char directory[] = "[textures]\n\"track.big1\"=\"images/\"";
        EXPECT(!ModManifestParse(directory, sizeof(directory) - 1, &manifest));
        EXPECT(!ModManifestParse(NULL, 0, &manifest));
        EXPECT(manifest.schemaVersion == 0 && manifest.textureCount == 0);
        EXPECT(manifest.error == RAGE_MOD_MANIFEST_INVALID);
        EXPECT(!ModManifestParse(valid, sizeof(valid) - 1, NULL));
        EXPECT(ModManifestParse(valid, sizeof(valid) - 1, &manifest));
        EXPECT(manifest.error == RAGE_MOD_MANIFEST_OK && manifest.schemaVersion == 1);
        EXPECT(strstr(ModManifestErrorString(RAGE_MOD_MANIFEST_UNSUPPORTED_VERSION),
                      "unsupported") != NULL);
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
