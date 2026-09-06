#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/render_material.h"
#include "render/authored_car_surface.h"

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
    original = material;
    EXPECT(!RenderMaterialParse(invalid, sizeof(invalid) - 1,
                                    8, 0, &material));
    EXPECT(memcmp(&material, &original, sizeof(material)) == 0);
    EXPECT(!RenderMaterialParse(overflowingIndex,
                                sizeof(overflowingIndex) - 1,
                                0, 0, &material));
    EXPECT(memcmp(&material, &original, sizeof(material)) == 0);
    EXPECT(!RenderMaterialParse(v5, sizeof(v5) - 1, 3, 9, &material));
    EXPECT(memcmp(&material, &original, sizeof(material)) == 0);

    RenderMaterialDefault(&material);
    material.roughness = 0.35f;
    original = material;
    EXPECT(!RenderMaterialParseProperties(
        "lit invalid 0.1 0.2 1 1 1 1 0 0 0",
        sizeof("lit invalid 0.1 0.2 1 1 1 1 0 0 0") - 1, &material));
    EXPECT(memcmp(&material, &original, sizeof(material)) == 0);
    {
        RageRenderMaterial glass, paint, rubber, metal;
        EXPECT(RenderMaterialParse(v5, sizeof(v5) - 1, 3, 0, &original));
        glass = paint = rubber = metal = original;
        AuthoredCarSurfaceApply(RAGE_CAR_SURFACE_GLASS, &glass);
        AuthoredCarSurfaceApply(RAGE_CAR_SURFACE_PAINT, &paint);
        AuthoredCarSurfaceApply(RAGE_CAR_SURFACE_RUBBER, &rubber);
        AuthoredCarSurfaceApply(RAGE_CAR_SURFACE_METAL, &metal);
        EXPECT(glass.roughness < paint.roughness && paint.roughness < rubber.roughness);
        EXPECT(glass.metallic == 0 && rubber.metallic == 0);
        EXPECT(metal.metallic > paint.metallic);
        EXPECT(PathEquals(glass.baseColorTexture, "car.rgba"));
        EXPECT(PathEquals(paint.paintMask, "paint.rpaint"));
        EXPECT(glass.alphaMode == original.alphaMode);
        EXPECT(glass.baseColorFactor[3] == original.baseColorFactor[3]);
        {
            unsigned surface;
            for (surface = 0; surface < RAGE_CAR_SURFACE_COUNT; surface++) {
                material = original;
                EXPECT(AuthoredCarSurfaceResolve(surface,
                    "lit opaque 0.7 0.6 0.5 0.4 0.3 1 0 0 0", &material));
                EXPECT(material.roughness == 0.7f);
                EXPECT(material.metallic == 0.6f);
                EXPECT(material.baseColorFactor[0] == 0.5f);
                EXPECT(PathEquals(material.baseColorTexture, "car.rgba"));
                EXPECT(PathEquals(material.paintMask, "paint.rpaint"));
                material = original;
                EXPECT(!AuthoredCarSurfaceResolve(surface, "invalid", &material));
                EXPECT(memcmp(&material, &original, sizeof(material)) == 0);
            }
            material = original;
            EXPECT(AuthoredCarSurfaceResolve(RAGE_CAR_SURFACE_GLASS, NULL, &material));
            EXPECT(memcmp(&material, &glass, sizeof(material)) == 0);
        }
        material = original;
        AuthoredCarSurfaceApply(RAGE_CAR_SURFACE_ORIGINAL, &material);
        EXPECT(memcmp(&material, &original, sizeof(material)) == 0);
        {
            uint8_t pixels[] = {240,240,240,255, 5,8,12,0};
            uint8_t saved[sizeof(pixels)];
            memcpy(saved, pixels, sizeof(pixels));
            AuthoredCarSurfaceTexture(RAGE_CAR_SURFACE_PAINT, pixels, sizeof(pixels));
            EXPECT(memcmp(saved, pixels, sizeof(pixels)) == 0);
            AuthoredCarSurfaceTexture(RAGE_CAR_SURFACE_GLASS, pixels, sizeof(pixels));
            EXPECT(memcmp(pixels, pixels + 4, 3) == 0);
            EXPECT(pixels[0] == 18 && pixels[1] == 25 && pixels[2] == 32);
            EXPECT(pixels[3] == 255 && pixels[7] == 0);
        }
        {
            static uint8_t atlas[256 * 256 * 4];
            unsigned x, y;
            memset(atlas, 213, sizeof(atlas));
            AuthoredCarSurfaceTexture(RAGE_CAR_SURFACE_GLASS, atlas, sizeof(atlas));
            for (y = 0; y < 256; ++y) for (x = 0; x < 256; ++x) {
                const uint8_t *p = atlas + (y * 256 + x) * 4;
                int banner = x >= 8 && x < 56 && y >= 55 && y < 63;
                EXPECT(p[0] == (banner ? 213 : 18));
                EXPECT(p[1] == (banner ? 213 : 25));
                EXPECT(p[2] == (banner ? 213 : 32));
                EXPECT(p[3] == 213);
            }
            memset(atlas, 213, sizeof(atlas));
            AuthoredCarSurfaceTexture(RAGE_CAR_SURFACE_DECAL, atlas, sizeof(atlas));
            for (y = 0; y < 256; ++y) for (x = 0; x < 256; ++x) {
                const uint8_t *p = atlas + (y * 256 + x) * 4;
                int canvas = x >= 64 && x < 128 && y >= 48 && y < 112;
                EXPECT(p[0] == (canvas ? 213 : 0));
                EXPECT(p[3] == (canvas ? 213 : 0));
            }
        }
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
