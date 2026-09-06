#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/mod_manifest.h"

static int failures;
#define EXPECT(value) do { if (!(value)) { failures++;                         \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__,  \
            #value);                                                           \
} } while (0)

static void test_dependency_order(void) {
    RageModManifest *storage = calloc(RAGE_MOD_MAX_SELECTED, sizeof(*storage));
    EXPECT(storage != NULL);
    if (!storage) return;
    const RageModManifest *selected[RAGE_MOD_MAX_SELECTED];
    for (unsigned i = 0; i < RAGE_MOD_MAX_SELECTED; ++i) selected[i] = &storage[i];
    const char *texts[] = {
        "[mod]\nid=\"addon\"\nrequires=[\"left\",\"right\"]",
        "[mod]\nid=\"left\"\nrequires=[\"base\"]",
        "[mod]\nid=\"right\"\nrequires=[\"base\"]",
        "[mod]\nid=\"base\""};
    for (unsigned i = 0; i < 4; ++i)
        EXPECT(ModManifestParse(texts[i], strlen(texts[i]), &storage[i]));
    RageModOrder order;
    EXPECT(ModManifestBuildOrder(selected, 4, &order));
    EXPECT(order.count == 4 && order.indices[0] == 3 && order.indices[1] == 1 &&
           order.indices[2] == 2 && order.indices[3] == 0);
    RageModOrder repeated;
    EXPECT(ModManifestBuildOrder(selected, 4, &repeated));
    EXPECT(memcmp(&order, &repeated, sizeof(order)) == 0);
    EXPECT(!ModManifestBuildOrder(selected, 3, &order));
    EXPECT(order.error == RAGE_MOD_ORDER_MISSING_REQUIREMENT && order.modIndex == 1 &&
           order.requirementIndex == 0 && order.count == 0);
    storage[3].requirementCount = 1;
    strcpy(storage[3].requirements[0], "addon");
    EXPECT(!ModManifestBuildOrder(selected, 4, &order));
    EXPECT(order.error == RAGE_MOD_ORDER_CYCLE && order.count == 0);
    storage[3].requirementCount = 0;
    strcpy(storage[2].id, "left");
    EXPECT(!ModManifestBuildOrder(selected, 4, &order));
    EXPECT(order.error == RAGE_MOD_ORDER_DUPLICATE_ID && order.modIndex == 2);
    for (unsigned i = 0; i < 4; ++i) {
        EXPECT(ModManifestParse(texts[i], strlen(texts[i]), &storage[i]));
        storage[i].requirementCount = 0;
    }
    EXPECT(ModManifestBuildOrder(selected, 4, &order));
    for (unsigned i = 0; i < 4; ++i) EXPECT(order.indices[i] == i);
    storage[0].id[0] = 0;
    EXPECT(ModManifestBuildOrder(selected, 1, &order));
    EXPECT(!ModManifestBuildOrder(selected, 4, &order));
    EXPECT(order.error == RAGE_MOD_ORDER_INVALID && order.count == 0);
    EXPECT(ModManifestBuildOrder(NULL, 0, &order) && order.count == 0);
    EXPECT(!ModManifestBuildOrder(NULL, 1, &order));
    EXPECT(!ModManifestBuildOrder(selected, RAGE_MOD_MAX_SELECTED + 1, &order));
    EXPECT(!ModManifestBuildOrder(selected, 1, NULL));
    for (unsigned i = 0; i < RAGE_MOD_MAX_SELECTED; ++i) {
        char text[96];
        int length = snprintf(text, sizeof(text), "[mod]\nid=\"m%u\"", i);
        EXPECT(ModManifestParse(text, (size_t)length, &storage[i]));
        if (i + 1 < RAGE_MOD_MAX_SELECTED) {
            storage[i].requirementCount = 1;
            snprintf(storage[i].requirements[0], sizeof(storage[i].requirements[0]), "m%u", i + 1);
        }
    }
    EXPECT(ModManifestBuildOrder(selected, RAGE_MOD_MAX_SELECTED, &order));
    EXPECT(order.count == RAGE_MOD_MAX_SELECTED);
    for (unsigned i = 0; i < RAGE_MOD_MAX_SELECTED; ++i)
        EXPECT(order.indices[i] == RAGE_MOD_MAX_SELECTED - 1 - i);
    strcpy(storage[0].requirements[0], "m0");
    EXPECT(!ModManifestBuildOrder(selected, 1, &order) && order.error == RAGE_MOD_ORDER_CYCLE);
    free(storage);
}

static void test_resolution(void) {
    static const char text[] =
        "[textures]\n\"track.a.variant.1\"=\"first.png\"\n"
        "\"track.a.variant.1\"=\"exact.png\"\n\"track.a\"=\"base.png\"\n"
        "[materials]\n\"track.a\"=\"unlit blend 0.2 0 1 1 1 1 0.4 0.3 0.2\"\n";
    RageModManifest *manifest = malloc(sizeof(*manifest));
    EXPECT(manifest != NULL);
    if (manifest == NULL) return;
    EXPECT(ModManifestParse(text, sizeof(text) - 1, manifest));
    char query[] = "track.a.variant.1";
    RageModResolution result = ModManifestResolve(manifest, query, "track.a", 3);
    EXPECT(result.texture == &manifest->textures[1]);
    EXPECT(result.material == &manifest->materials[0]);
    memset(query, 'x', sizeof(query) - 1);
    EXPECT(result.texture != NULL && strcmp(result.texture->key, "track.a.variant.1") == 0);
    result = ModManifestResolve(manifest, "missing", "track.a", 3);
    EXPECT(result.texture == &manifest->textures[2] && result.material == &manifest->materials[0]);
    result = ModManifestResolve(manifest, "track.a.variant.1", "track.a", RAGE_MOD_RESOLVE_TEXTURE);
    EXPECT(result.texture == &manifest->textures[1] && result.material == NULL);
    result = ModManifestResolve(manifest, "track.a.variant.1", "track.a", RAGE_MOD_RESOLVE_MATERIAL);
    EXPECT(result.texture == NULL && result.material == &manifest->materials[0]);
    result = ModManifestResolve(manifest, NULL, NULL, 3);
    EXPECT(result.texture == NULL && result.material == NULL);
    manifest->textureCount = RAGE_MOD_MANIFEST_MAX_TEXTURES + 1;
    EXPECT(ModManifestFindTexture(manifest, "track.a") == NULL);
    EXPECT(ModManifestFindMaterialProperties(manifest, "track.a") == NULL);
    result = ModManifestResolve(manifest, "track.a", "track.a", 3);
    EXPECT(result.texture == NULL && result.material == NULL);
    result = ModManifestResolve(NULL, "track.a", "track.a", 3);
    EXPECT(result.texture == NULL && result.material == NULL);
    static const char materialExact[] =
        "[materials]\n\"track.a.variant.1\"=\"unlit blend 0.2 0 1 1 1 1 0 0 0\"\n"
        "\"track.a\"=\"unlit blend 0.5 0 1 1 1 1 0 0 0\"\n";
    EXPECT(ModManifestParse(materialExact, sizeof(materialExact) - 1, manifest));
    result = ModManifestResolve(manifest, "track.a.variant.1", "track.a", 3);
    EXPECT(result.texture == NULL && result.material == &manifest->materials[0]);
    result = ModManifestResolve(manifest, "track.a.variant.1", "track.a", 0);
    EXPECT(result.texture == NULL && result.material == NULL);
    result = ModManifestResolve(manifest, "track.a.variant.1", "track.a", 4);
    EXPECT(result.texture == NULL && result.material == NULL);
    manifest->schemaVersion++;
    EXPECT(ModManifestFindMaterialProperties(manifest, "track.a") == NULL);
    manifest->schemaVersion = RAGE_MOD_MANIFEST_SCHEMA_VERSION;
    manifest->materialCount = RAGE_MOD_MANIFEST_MAX_MATERIALS + 1;
    EXPECT(ModManifestFindMaterialProperties(manifest, "track.a") == NULL);
    free(manifest);
}

int main(void) {
    test_dependency_order();
    test_resolution();
    {
        RageModManifest *requirements = malloc(sizeof(*requirements));
        EXPECT(requirements != NULL);
        if (requirements == NULL) return EXIT_FAILURE;
        const char *validRequirements = "[mod]\nid=\"addon\"\nrequires=[\"base-pack\", \"cars.hd\",] # comment\n";
        EXPECT(ModManifestParse(validRequirements, strlen(validRequirements), requirements));
        EXPECT(requirements->requirementCount == 2);
        EXPECT(strcmp(requirements->requirements[0], "base-pack") == 0);
        EXPECT(strcmp(requirements->requirements[1], "cars.hd") == 0);
        const char *invalidRequirements[] = {
            "[mod]\nrequires=\"base\"", "[mod]\nrequires=[\"base\",\"base\"]",
            "[mod]\nrequires=[\"base\"", "[mod]\nrequires=[\"../base\"]",
            "[mod]\nrequires=[]\nrequires=[]", "[mod]\nrequires=[\"\"]",
            "[mod]\nrequires=[\"a\" \"b\"]"};
        for (unsigned i = 0; i < sizeof(invalidRequirements) / sizeof(invalidRequirements[0]); ++i) {
            EXPECT(!ModManifestParse(invalidRequirements[i], strlen(invalidRequirements[i]), requirements));
            EXPECT(requirements->requirementCount == 0 && requirements->textureCount == 0);
        }
        EXPECT(ModManifestParse("[mod]\nrequires=[]", strlen("[mod]\nrequires=[]"), requirements));
        EXPECT(requirements->requirementCount == 0);
        for (unsigned count = RAGE_MOD_MANIFEST_MAX_REQUIREMENTS;
             count <= RAGE_MOD_MANIFEST_MAX_REQUIREMENTS + 1; ++count) {
            char bounded[512] = "[mod]\nrequires=[";
            size_t used = strlen(bounded);
            for (unsigned i = 0; i < count; ++i)
                used += (size_t)snprintf(bounded + used, sizeof(bounded) - used,
                                        "%s\"pack%u\"", i ? "," : "", i);
            bounded[used++] = ']';
            EXPECT(ModManifestParse(bounded, used, requirements) ==
                   (count == RAGE_MOD_MANIFEST_MAX_REQUIREMENTS));
        }
        free(requirements);
    }
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
    static RageModManifest manifest;
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

    {
        const char *meshes = "[meshes]\n\"car.player.10.part.0\" = \"meshes/body.rmesh\"\n"
                             "\"car.player.10.part.0\" = \"meshes/revised.rmesh\"\n";
        const char *bad[] = {"../body.rmesh", "meshes/../body.rmesh",
                             "meshes/body.obj", "/meshes/body.rmesh"};
        size_t i;
        EXPECT(ModManifestParse(meshes, strlen(meshes), &manifest));
        EXPECT(manifest.meshCount == 2);
        path = ModManifestFindMesh(&manifest, "car.player.10.part.0");
        EXPECT(path && strcmp(path, "meshes/revised.rmesh") == 0);
        EXPECT(ModManifestFindMesh(&manifest, "missing") == NULL);
        for (i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
            char input[256];
            snprintf(input, sizeof(input), "[meshes]\n\"car.player.10.part.0\" = \"%s\"\n", bad[i]);
            EXPECT(!ModManifestParse(input, strlen(input), &manifest));
            EXPECT(manifest.meshCount == 0 && manifest.errorLine == 2);
        }
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
