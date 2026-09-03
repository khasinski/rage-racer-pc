#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/render_material.h"

static int failures;
#define EXPECT(value) do { if (!(value)) { failures++;                      \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__, \
            #value);                                                        \
} } while (0)

static int PathEquals(RageRenderMaterialPath path, const char *expected) {
    return path.length == strlen(expected) &&
           memcmp(path.text, expected, path.length) == 0;
}

int main(void) {
    static const char v4[] =
        "# rage-rmat v4\n0 red.rgba blue.rgba\n";
    static const char v5[] =
        "# rage-rmat v5\n3 car.rgba car-alt.rgba | paint.rpaint\n";
    static const char v6[] =
        "# rage-rmat v6\n"
        "8 sign.rgba | - | unlit mask 0.25 0.5 0.8 0.7 0.6 0.5 1 0.2 0.1\n";
    static const char invalid[] =
        "# rage-rmat v6\n"
        "8 sign.rgba | - | glow mask 0.25 0.5 1 1 1 1 0 0 0\n";
    static const char overflowingIndex[] =
        "# rage-rmat v4\n4294967296 wrapped-to-zero.rgba\n";
    RageRenderMaterial material;
    RageRenderMaterial original;

    EXPECT(RenderMaterialParse(v4, sizeof(v4) - 1, 0, 1, &material));
    EXPECT(PathEquals(material.baseColorTexture, "blue.rgba"));
    EXPECT(material.shading == RAGE_RENDER_MATERIAL_SHADING_INHERIT);
    EXPECT(material.roughness == 1.0f && material.metallic == 0.0f);

    EXPECT(RenderMaterialParse(v5, sizeof(v5) - 1, 3, 0, &material));
    EXPECT(PathEquals(material.baseColorTexture, "car.rgba"));
    EXPECT(PathEquals(material.paintMask, "paint.rpaint"));
    EXPECT(!RenderMaterialParse(v5, sizeof(v5) - 1, 3, 2, &material));

    EXPECT(RenderMaterialParse(v6, sizeof(v6) - 1, 8, 0, &material));
    EXPECT(PathEquals(material.baseColorTexture, "sign.rgba"));
    EXPECT(material.paintMask.length == 0);
    EXPECT(material.shading == RAGE_RENDER_MATERIAL_SHADING_UNLIT);
    EXPECT(material.alphaMode == RAGE_RENDER_MATERIAL_ALPHA_MASK);
    EXPECT(material.roughness == 0.25f && material.metallic == 0.5f);
    EXPECT(material.baseColorFactor[0] == 0.8f);
    EXPECT(material.baseColorFactor[3] == 0.5f);
    EXPECT(material.emissiveFactor[0] == 1.0f);
    EXPECT(!RenderMaterialParse(invalid, sizeof(invalid) - 1,
                                    8, 0, &material));
    EXPECT(!RenderMaterialParse(overflowingIndex,
                                sizeof(overflowingIndex) - 1,
                                0, 0, &material));

    RenderMaterialDefault(&material);
    material.roughness = 0.35f;
    original = material;
    EXPECT(!RenderMaterialParseProperties(
        "lit invalid 0.1 0.2 1 1 1 1 0 0 0",
        sizeof("lit invalid 0.1 0.2 1 1 1 1 0 0 0") - 1, &material));
    EXPECT(memcmp(&material, &original, sizeof(material)) == 0);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
